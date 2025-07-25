#include "../shared/common.h"
#include "../shared/protocol.h"
#include "../shared/ipc.h"
#include <winsock2.h>
#include <iostream>
#include <windows.h>

class ClientServer {
    HANDLE hIOCP_;
    SOCKET listenSocket_;

    struct ClientContext {
        OVERLAPPED overlapped;
        SOCKET socket;
        WSABUF wsa_buf;
        char buffer[1024];
        IPCChannel* logger_channel;
    };

public:
    void run() {
        // 1. Инициализация Winsock
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        // 2. Создание IOCP
        hIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

        // 3. Настройка слушающего сокета
        listenSocket_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        sockaddr_in serverAddr = { 0 };
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        serverAddr.sin_port = htons(CLIENT_SERVER_PORT);
        bind(listenSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr));
        listen(listenSocket_, SOMAXCONN);

        // 4. Запуск рабочих потоков
        const int numThreads = 4;
        HANDLE threads[numThreads];
        for (int i = 0; i < numThreads; ++i) {
            threads[i] = CreateThread(NULL, 0, &ClientServer::worker_thread, this, 0, NULL);
        }

        std::cout << "Client server started on port " << CLIENT_SERVER_PORT << "\n";

        // 5. Основной цикл принятия подключений
        while (true) {
            SOCKET clientSocket = accept(listenSocket_, NULL, NULL);
            handle_new_connection(clientSocket);
        }
    }

private:
    void handle_new_connection(SOCKET clientSocket) {
        ClientContext* context = new ClientContext();
        ZeroMemory(context, sizeof(ClientContext));
        context->socket = clientSocket;
        context->wsa_buf.buf = context->buffer;
        context->wsa_buf.len = sizeof(context->buffer);
        context->logger_channel = new SocketIPC("127.0.0.1", LOGGER_SERVER_PORT);

        // Привязка к IOCP
        CreateIoCompletionPort((HANDLE)clientSocket, hIOCP_, (ULONG_PTR)context, 0);

        // Начало асинхронного чтения
        DWORD flags = 0;
        WSARecv(clientSocket, &context->wsa_buf, 1, NULL, &flags, &context->overlapped, NULL);
    }

    static DWORD WINAPI worker_thread(LPVOID lpParam) {
        ClientServer* self = static_cast<ClientServer*>(lpParam);
        DWORD bytes_transferred;
        ULONG_PTR completion_key;
        LPOVERLAPPED overlapped;

        while (GetQueuedCompletionStatus(self->hIOCP_, &bytes_transferred,
            &completion_key, &overlapped, INFINITE)) {
            ClientContext* context = CONTAINING_RECORD(overlapped, ClientContext, overlapped);

            if (bytes_transferred == 0) {
                delete context;
                continue;
            }

            // Отправка в логгер
            MessageHeader header{ LOG_REQUEST, bytes_transferred };
            context->logger_channel->async_send(&header, sizeof(header), &context->overlapped);
            context->logger_channel->async_send(context->buffer, bytes_transferred, &context->overlapped);

            // Подготовка к следующему приему
            context->wsa_buf.buf = context->buffer;
            context->wsa_buf.len = sizeof(context->buffer);
            DWORD flags = 0;
            WSARecv(context->socket, &context->wsa_buf, 1, NULL, &flags, &context->overlapped, NULL);
        }
        return 0;
    }
};

int main() {
    ClientServer server;
    server.run();
    return 0;
}