#include "../shared/common.h"
#include "../shared/protocol.h"
#include "../shared/ipc.h"
#include <winsock2.h>
#include <iostream>
#include <windows.h>

class Monitor {
    int client_connections = 0;
    int log_entries = 0;
    HANDLE hIOCP_;

    struct MonitorContext {
        OVERLAPPED overlapped;
        WSABUF wsa_buf;
        char buffer[sizeof(MessageHeader)];
    };

public:
    void run() {
        // 1. Инициализация IOCP
        hIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (!hIOCP_) {
            std::cerr << "Failed to create IOCP\n";
            return;
        }

        // 2. Создание сервера
        SocketIPC channel("0.0.0.0", MONITOR_PORT, true);

        // 3. Создание рабочих потоков
        const int num_threads = 2;
        HANDLE threads[num_threads];
        for (int i = 0; i < num_threads; ++i) {
            threads[i] = CreateThread(NULL, 0, &Monitor::worker_thread, this, 0, NULL);
        }

        // 4. Начало асинхронного приема
        MonitorContext* context = new MonitorContext();
        ZeroMemory(context, sizeof(MonitorContext));
        context->wsa_buf.buf = context->buffer;
        context->wsa_buf.len = sizeof(context->buffer);

        DWORD flags = 0;
        if (channel.async_receive(context->buffer, sizeof(MessageHeader), &context->overlapped)) {
            std::cout << "Monitor started on port " << MONITOR_PORT << "\n";
        }
        else {
            std::cerr << "Failed to start async receive\n";
        }

        // Главный поток просто ждет
        WaitForMultipleObjects(num_threads, threads, TRUE, INFINITE);
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
                delete context;
                continue;
            }

            MessageHeader* header = reinterpret_cast<MessageHeader*>(context->buffer);
            if (header->type == STATS_UPDATE) {
                self->log_entries++;
                std::cout << "Статистика: "
                    << self->client_connections << " подключений, "
                    << self->log_entries << " записей лога\n";
            }

            // Подготовка к следующему приему
            ZeroMemory(context, sizeof(MonitorContext));
            context->wsa_buf.buf = context->buffer;
            context->wsa_buf.len = sizeof(context->buffer);

            DWORD flags = 0;
            SocketIPC* channel = reinterpret_cast<SocketIPC*>(completion_key);
            channel->async_receive(context->buffer, sizeof(MessageHeader), &context->overlapped);
        }
        return 0;
    }
};

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    Monitor monitor;
    monitor.run();

    WSACleanup();
    return 0;
}