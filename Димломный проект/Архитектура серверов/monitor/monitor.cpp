#include "../shared/common.h"
#include "../shared/protocol.h"
#include "../shared/ipc.h"
#include <winsock2.h>
#include <iostream>
#include <windows.h>

class Monitor {
    int client_count = 0;
    int message_count = 0;
    HANDLE hIOCP_;

    struct MonitorContext {
        OVERLAPPED overlapped;
        WSABUF wsa_buf;
        char buffer[1024];
        MessageHeader header;
        SocketIPC* channel;
    };

public:
    void run() {
        hIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (!hIOCP_) {
            std::cerr << "Failed to create IOCP: " << GetLastError() << "\n";
            return;
        }

        try {
            // Создаем слушающий сокет
            SOCKET listen_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
            if (listen_socket == INVALID_SOCKET) {
                throw std::runtime_error("Socket creation failed");
            }

            // Настройка SO_REUSEADDR
            int optval = 1;
            if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
                closesocket(listen_socket);
                throw std::runtime_error("setsockopt failed");
            }

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(MONITOR_SERVER_PORT);
            server_addr.sin_addr.s_addr = INADDR_ANY;

            if (bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
                closesocket(listen_socket);
                throw std::runtime_error("Bind failed");
            }

            if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
                closesocket(listen_socket);
                throw std::runtime_error("Listen failed");
            }

            std::cout << "Monitor server started on port " << MONITOR_SERVER_PORT << "\n";

            const int num_threads = 2;
            HANDLE threads[num_threads];
            for (int i = 0; i < num_threads; ++i) {
                threads[i] = CreateThread(NULL, 0, &Monitor::worker_thread, this, 0, NULL);
            }

            // Основной цикл принятия подключений
            while (true) {
                sockaddr_in client_addr{};
                int addr_len = sizeof(client_addr);
                SOCKET client_socket = accept(listen_socket, (sockaddr*)&client_addr, &addr_len);

                if (client_socket == INVALID_SOCKET) {
                    std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
                    continue;
                }

                std::cout << "New monitor client connected: "
                    << inet_ntoa(client_addr.sin_addr) << ":"
                    << ntohs(client_addr.sin_port) << "\n";

                // Создаем новый канал для клиента
                SocketIPC* channel = new SocketIPC(client_socket);
                CreateIoCompletionPort((HANDLE)client_socket, hIOCP_, (ULONG_PTR)channel, 0);

                // Инициализируем контекст для приема данных
                MonitorContext* context = new MonitorContext();
                ZeroMemory(context, sizeof(MonitorContext));
                context->channel = channel;
                context->wsa_buf.buf = reinterpret_cast<char*>(&context->header);
                context->wsa_buf.len = sizeof(MessageHeader);

                if (!channel->async_receive(&context->header, sizeof(MessageHeader), &context->overlapped)) {
                    std::cerr << "Failed to start async receive\n";
                    delete context;
                    delete channel;
                }
            }

            WaitForMultipleObjects(num_threads, threads, TRUE, INFINITE);
            closesocket(listen_socket);
        }
        catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }

private:
    static DWORD WINAPI worker_thread(LPVOID lpParam) {
        Monitor* self = static_cast<Monitor*>(lpParam);
        DWORD bytes_transferred;
        ULONG_PTR completion_key;
        LPOVERLAPPED overlapped;

        while (GetQueuedCompletionStatus(self->hIOCP_, &bytes_transferred,
            &completion_key, &overlapped, INFINITE)) {
            MonitorContext* context = CONTAINING_RECORD(overlapped, MonitorContext, overlapped);

            if (bytes_transferred == 0) {
                std::cout << "Monitor client disconnected\n";
                delete context->channel;
                delete context;
                continue;
            }

            if (context->wsa_buf.buf == reinterpret_cast<char*>(&context->header)) {
                if (context->header.type == STATS_UPDATE) {
                    MonitorContext* data_context = new MonitorContext();
                    data_context->channel = context->channel;
                    data_context->wsa_buf.buf = data_context->buffer;
                    data_context->wsa_buf.len = context->header.size;

                    if (!context->channel->async_receive(data_context->buffer, context->header.size, &data_context->overlapped)) {
                        std::cerr << "Failed to receive data\n";
                        delete data_context;
                    }
                }
                delete context;
            }
            else {
                self->message_count++;
                std::cout << "Monitor update [" << self->message_count << "]: "
                    << std::string(context->buffer, context->header.size) << "\n";

                // Подготовка к следующему сообщению
                context->wsa_buf.buf = reinterpret_cast<char*>(&context->header);
                context->wsa_buf.len = sizeof(MessageHeader);

                if (!context->channel->async_receive(&context->header, sizeof(MessageHeader), &context->overlapped)) {
                    std::cerr << "Failed to receive next header\n";
                    delete context->channel;
                    delete context;
                }
            }
        }
        return 0;
    }
};

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    Monitor monitor;
    monitor.run();

    WSACleanup();
    return 0;
}