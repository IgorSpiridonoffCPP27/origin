#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <ws2tcpip.h>
#include <fstream> 
#include <mutex>
std::mutex logMutex;

#pragma warning(disable: 4996)

#define MAX_CLIENTS 100
#define WORKER_THREADS 4

HANDLE hIOCP;
SOCKET ListenSocket;

std::vector<SOCKET> Clients;

// Структура для хранения данных клиента
struct ClientContext {
    OVERLAPPED Overlapped;
    SOCKET Socket;
    WSABUF DataBuf;
    char Buffer[256];
};

DWORD WINAPI WorkerThread(LPVOID lpParam) {
    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped;

    while (GetQueuedCompletionStatus(hIOCP, &bytesTransferred, &completionKey, &overlapped, INFINITE)) {
        ClientContext* context = CONTAINING_RECORD(overlapped, ClientContext, Overlapped);
        
        if (bytesTransferred == 0) {
            // Клиент отключился
            closesocket(context->Socket);
            delete context;
            continue;
        }

        // Обработка полученных данных
        context->Buffer[bytesTransferred] = '\0';


        // Запись в файл с обработкой ошибок
        {
            std::lock_guard<std::mutex> lock(logMutex);
            std::ofstream outFile("log.txt", std::ios::app | std::ios::binary);
            if (outFile.is_open()) {
                outFile << "Клиент [" << context->Socket << "]: " << context->Buffer << "\n";
            }
            else {
                std::cerr << "Ошибка записи в log.txt\n";
            }
        }

        std::cout << "Received: " << context->Buffer << std::endl;

        // Рассылка всем клиентам
        for (SOCKET client : Clients) {
            if (client != context->Socket && client != INVALID_SOCKET) {
                WSASend(client, &context->DataBuf, 1, NULL, 0, NULL, NULL);
            }
        }

        // Запускаем новое асинхронное чтение
        DWORD flags = 0;
        WSARecv(context->Socket, &context->DataBuf, 1, NULL, &flags, &context->Overlapped, NULL);
    }
    return 0;
}

int main() {
    setlocale(LC_ALL, "Rus");
    // 1. Инициализация Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 2. Создание сокета
    ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    // 3. Привязка и прослушивание
    sockaddr_in serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(1111);
    bind(ListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(ListenSocket, SOMAXCONN);

    // 4. Создание IOCP
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, WORKER_THREADS);

    // 5. Запуск рабочих потоков
    HANDLE threads[WORKER_THREADS];
    for (int i = 0; i < WORKER_THREADS; ++i) {
        threads[i] = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
    }
    std::cout << "Сервер создан" << std::endl;
    // 6. Основной цикл accept
    while (true) {
        SOCKET clientSocket = accept(ListenSocket, NULL, NULL);
        Clients.push_back(clientSocket);

        // Создаем контекст для клиента
        ClientContext* context = new ClientContext();
        ZeroMemory(context, sizeof(ClientContext));
        context->Socket = clientSocket;
        context->DataBuf.buf = context->Buffer;
        context->DataBuf.len = sizeof(context->Buffer);

        // Привязываем сокет к IOCP
        CreateIoCompletionPort((HANDLE)clientSocket, hIOCP, (ULONG_PTR)context, 0);

        // Начинаем асинхронное чтение
        DWORD flags = 0;
        WSARecv(clientSocket, &context->DataBuf, 1, NULL, &flags, &context->Overlapped, NULL);
    }

    return 0;
}