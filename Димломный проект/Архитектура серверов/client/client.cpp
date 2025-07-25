#include "../shared/common.h"
#include "../shared/protocol.h"
#include "../shared/ipc.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <windows.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

class Client {
    HANDLE hIOCP_;
    SOCKET socket_;
    bool running_;

    struct ClientContext {
        OVERLAPPED overlapped;
        WSABUF wsa_buf;
        char buffer[1024];
    };

public:
    Client() : socket_(INVALID_SOCKET), running_(false) {}

    void run() {
        // 1. Инициализация Winsock
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        // 2. Создание IOCP
        hIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (!hIOCP_) {
            std::cerr << "Failed to create IOCP\n";
            return;
        }

        // 3. Подключение к серверу
        if (!connect_to_server("127.0.0.1", CLIENT_SERVER_PORT)) {
            return;
        }

        // 4. Запуск потока для приема сообщений
        running_ = true;
        std::thread receiver(&Client::receive_thread, this);

        // 5. Основной цикл отправки сообщений
        std::string input;
        while (running_) {
            std::cout << "Enter message (or 'exit' to quit): ";
            std::getline(std::cin, input);

            if (input == "exit") {
                running_ = false;
                break;
            }

            send_message(input);
        }

        // 6. Очистка ресурсов
        receiver.join();
        closesocket(socket_);
        WSACleanup();
    }

private:
    bool connect_to_server(const std::string& ip, int port) {
        socket_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (socket_ == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n";
            return false;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

        if (WSAConnect(socket_, (sockaddr*)&serverAddr, sizeof(serverAddr), NULL, NULL, NULL, NULL) == SOCKET_ERROR) {
            std::cerr << "Connection failed\n";
            closesocket(socket_);
            return false;
        }

        // Привязка сокета к IOCP
        CreateIoCompletionPort((HANDLE)socket_, hIOCP_, (ULONG_PTR)this, 0);
        std::cout << "Connected to server at " << ip << ":" << port << "\n";
        return true;
    }

    void send_message(const std::string& message) {
        ClientContext* context = new ClientContext();
        ZeroMemory(context, sizeof(ClientContext));
        context->wsa_buf.buf = const_cast<char*>(message.c_str());
        context->wsa_buf.len = (ULONG)message.size();

        DWORD bytes_sent;
        if (WSASend(socket_, &context->wsa_buf, 1, &bytes_sent, 0, &context->overlapped, NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() != WSA_IO_PENDING) {
                std::cerr << "WSASend failed\n";
                delete context;
            }
        }
    }

    void receive_thread() {
        ClientContext* context = new ClientContext();
        ZeroMemory(context, sizeof(ClientContext));
        context->wsa_buf.buf = context->buffer;
        context->wsa_buf.len = sizeof(context->buffer);

        DWORD flags = 0;
        DWORD bytes_received;
        if (WSARecv(socket_, &context->wsa_buf, 1, &bytes_received, &flags, &context->overlapped, NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() != WSA_IO_PENDING) {
                std::cerr << "WSARecv failed\n";
                delete context;
                return;
            }
        }

        DWORD bytes_transferred;
        ULONG_PTR completion_key;
        LPOVERLAPPED overlapped;

        while (running_) {
            if (GetQueuedCompletionStatus(hIOCP_, &bytes_transferred, &completion_key, &overlapped, 100)) {
                if (bytes_transferred == 0) {
                    std::cerr << "Server disconnected\n";
                    running_ = false;
                    break;
                }

                ClientContext* ctx = CONTAINING_RECORD(overlapped, ClientContext, overlapped);
                ctx->buffer[bytes_transferred] = '\0';
                std::cout << "Received: " << ctx->buffer << "\n";

                // Подготовка к следующему приему
                ZeroMemory(ctx, sizeof(ClientContext));
                ctx->wsa_buf.buf = ctx->buffer;
                ctx->wsa_buf.len = sizeof(ctx->buffer);
                flags = 0;
                WSARecv(socket_, &ctx->wsa_buf, 1, NULL, &flags, &ctx->overlapped, NULL);
            }
        }
        delete context;
    }
};

int main() {
    Client client;
    client.run();
    return 0;
}