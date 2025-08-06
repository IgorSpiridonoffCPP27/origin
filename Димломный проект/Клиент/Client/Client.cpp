#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#pragma warning(disable: 4996)

HANDLE hIOCP;

struct ClientContext {
    OVERLAPPED Overlapped;
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
            std::cout << "Server disconnected\n";
            break;
        }

        context->Buffer[bytesTransferred] = '\0';
        std::cout << "Server: " << context->Buffer << std::endl;

        // Запускаем новое чтение
        DWORD flags = 0;
        WSARecv((SOCKET)completionKey, &context->DataBuf, 1, NULL, &flags, &context->Overlapped, NULL);
    }
    return 0;
}

int main() {
    setlocale(LC_ALL, "Rus");
    // 1. Инициализация Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 2. Создание сокета
    SOCKET clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    // 3. Подключение
    sockaddr_in serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(1111);
    connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    // 4. Создание IOCP
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);

    // 5. Привязка сокета к IOCP
    CreateIoCompletionPort((HANDLE)clientSocket, hIOCP, (ULONG_PTR)clientSocket, 0);

    // 6. Запуск потока обработки
    HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);

    // 7. Подготовка контекста
    ClientContext context;
    ZeroMemory(&context, sizeof(context));
    context.DataBuf.buf = context.Buffer;
    context.DataBuf.len = sizeof(context.Buffer);
    std::cout << "Клиент создан" << std::endl;



    // 8. Начало асинхронного чтения
    DWORD flags = 0;
    WSARecv(clientSocket, &context.DataBuf, 1, NULL, &flags, &context.Overlapped, NULL);

    // 9. Основной цикл отправки
    char msg[256];
    while (true) {
        std::cin.getline(msg, sizeof(msg));
        WSABUF wsaBuf = { strlen(msg) + 1, msg };
        WSASend(clientSocket, &wsaBuf, 1, NULL, 0, NULL, NULL);
    }

    return 0;
}