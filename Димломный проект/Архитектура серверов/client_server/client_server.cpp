#include "../shared/common.h"
#include "../shared/protocol.h"
#include "../shared/ipc.h"
#include <winsock2.h>
#include <iostream>
#include <windows.h>
#include <string>

class ClientServer {
    HANDLE hIOCP_;
    SOCKET listenSocket_;

    struct ClientContext {
        OVERLAPPED overlapped;
        SOCKET socket;
        WSABUF wsa_buf;
        char buffer[1024];
        IPCChannel* logger_channel;
        IPCChannel* monitor_channel;

        ~ClientContext() {
            delete logger_channel;
            delete monitor_channel;
        }
    };

public:
    void run() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed\n";
            return;
        }

        hIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (hIOCP_ == NULL) {
            std::cerr << "CreateIoCompletionPort failed: " << GetLastError() << "\n";
            WSACleanup();
            return;
        }

        listenSocket_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (listenSocket_ == INVALID_SOCKET) {
            std::cerr << "WSASocket failed: " << WSAGetLastError() << "\n";
            CloseHandle(hIOCP_);
            WSACleanup();
            return;
        }

        sockaddr_in serverAddr = { 0 };
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        serverAddr.sin_port = htons(CLIENT_SERVER_PORT);

        if (bind(listenSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "bind failed: " << WSAGetLastError() << "\n";
            closesocket(listenSocket_);
            CloseHandle(hIOCP_);
            WSACleanup();
            return;
        }

        if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "listen failed: " << WSAGetLastError() << "\n";
            closesocket(listenSocket_);
            CloseHandle(hIOCP_);
            WSACleanup();
            return;
        }

        const int numThreads = 4;
        HANDLE threads[numThreads];
        for (int i = 0; i < numThreads; ++i) {
            threads[i] = CreateThread(NULL, 0, &ClientServer::worker_thread, this, 0, NULL);
            if (!threads[i]) {
                std::cerr << "Failed to create thread: " << GetLastError() << "\n";
            }
        }

        std::cout << "Client server started on port " << CLIENT_SERVER_PORT << "\n";

        while (true) {
            SOCKET clientSocket = accept(listenSocket_, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "accept failed: " << WSAGetLastError() << "\n";
                continue;
            }
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

        // Подключение к логгеру с 5 попытками
        int attempts = 5;
        while (attempts-- > 0) {
            try {
                context->logger_channel = new SocketIPC("127.0.0.1", LOGGER_SERVER_PORT);
                std::cout << "Successfully connected to logger server\n";
                break;
            }
            catch (const std::exception& e) {
                if (attempts == 0) {
                    std::cerr << "Final connection attempt to logger failed: " << e.what() << "\n";
                    delete context;
                    return;
                }
                std::cerr << "Logger connection failed (" << attempts << " attempts left), retrying...\n";
                Sleep(2000); // Увеличиваем задержку между попытками
            }
        }

        // Подключение к монитору с повторными попытками
        attempts = 3;
        while (attempts-- > 0) {
            try {
                context->monitor_channel = new SocketIPC("127.0.0.1", MONITOR_SERVER_PORT);
                std::cout << "Connected to monitor server\n";
                break;
            }
            catch (const std::exception& e) {
                if (attempts == 0) {
                    std::cerr << "Failed to connect to monitor: " << e.what() << "\n";
                    delete context;
                    return;
                }
                std::cerr << "Monitor connection failed, retrying... (" << attempts << " attempts left)\n";
                Sleep(1000);
            }
        }

        if (!CreateIoCompletionPort((HANDLE)clientSocket, hIOCP_, (ULONG_PTR)context, 0)) {
            std::cerr << "CreateIoCompletionPort failed: " << GetLastError() << "\n";
            delete context;
            return;
        }

        DWORD flags = 0;
        if (WSARecv(clientSocket, &context->wsa_buf, 1, NULL, &flags, &context->overlapped, NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() != WSA_IO_PENDING) {
                std::cerr << "WSARecv failed: " << WSAGetLastError() << "\n";
                delete context;
            }
        }
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
                std::cout << "Client disconnected\n";
                delete context;
                continue;
            }

            context->buffer[bytes_transferred] = '\0';
            std::cout << "Received from client: " << context->buffer << "\n";

            try {
                // Отправка в логгер
                MessageHeader logger_header{ CLIENT_DATA, bytes_transferred };
                if (!context->logger_channel->async_send(&logger_header, sizeof(logger_header), &context->overlapped) ||
                    !context->logger_channel->async_send(context->buffer, bytes_transferred, &context->overlapped)) {
                    std::cerr << "Failed to send to logger\n";
                }

                // Отправка в монитор
                std::string monitor_msg = "Client activity: " + std::string(context->buffer, bytes_transferred);
                MessageHeader monitor_header{ STATS_UPDATE, static_cast<uint32_t>(monitor_msg.size()) };
                if (!context->monitor_channel->async_send(&monitor_header, sizeof(monitor_header), &context->overlapped) ||
                    !context->monitor_channel->async_send(monitor_msg.data(), monitor_msg.size(), &context->overlapped)) {
                    std::cerr << "Failed to send to monitor\n";
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
            }

            // Подготовка к следующему приему
            context->wsa_buf.buf = context->buffer;
            context->wsa_buf.len = sizeof(context->buffer);
            DWORD flags = 0;
            if (WSARecv(context->socket, &context->wsa_buf, 1, NULL, &flags, &context->overlapped, NULL) == SOCKET_ERROR) {
                if (WSAGetLastError() != WSA_IO_PENDING) {
                    std::cerr << "WSARecv failed: " << WSAGetLastError() << "\n";
                    delete context;
                }
            }
        }
        return 0;
    }
};

int main() {
    ClientServer server;
    server.run();
    return 0;
}