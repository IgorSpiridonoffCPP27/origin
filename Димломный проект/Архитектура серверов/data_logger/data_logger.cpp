#include "../shared/common.h"
#include "../shared/protocol.h"
#include "../shared/ipc.h"
#include <winsock2.h>
#include <iostream>
#include <windows.h>
#include <fstream>

class DataLogger {
    HANDLE hIOCP_;
    std::ofstream log_file_;

    struct LoggerContext {
        OVERLAPPED overlapped;
        WSABUF wsa_buf;
        char buffer[1024];
        MessageHeader header;
    };

public:
    void run() {
        // 1. Инициализация файла
        log_file_.open("server.log", std::ios::app | std::ios::binary);

        // 2. Создание IOCP
        hIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

        // 3. Запуск сервера
        SocketIPC channel("0.0.0.0", LOGGER_SERVER_PORT, true);

        // 4. Запуск рабочих потоков
        const int numThreads = 2;
        HANDLE threads[numThreads];
        for (int i = 0; i < numThreads; ++i) {
            threads[i] = CreateThread(NULL, 0, &DataLogger::worker_thread, this, 0, NULL);
        }

        // 5. Начало асинхронного приема
        LoggerContext* context = new LoggerContext();
        ZeroMemory(context, sizeof(LoggerContext));
        context->wsa_buf.buf = reinterpret_cast<char*>(&context->header);
        context->wsa_buf.len = sizeof(MessageHeader);

        channel.async_receive(&context->header, sizeof(MessageHeader), &context->overlapped);

        std::cout << "Data logger started on port " << LOGGER_SERVER_PORT << "\n";
        WaitForMultipleObjects(numThreads, threads, TRUE, INFINITE);
    }

private:
    static DWORD WINAPI worker_thread(LPVOID lpParam) {
        DataLogger* self = static_cast<DataLogger*>(lpParam);
        DWORD bytes_transferred;
        ULONG_PTR completion_key;
        LPOVERLAPPED overlapped;

        while (GetQueuedCompletionStatus(self->hIOCP_, &bytes_transferred,
            &completion_key, &overlapped, INFINITE)) {
            LoggerContext* context = CONTAINING_RECORD(overlapped, LoggerContext, overlapped);

            if (bytes_transferred == 0) {
                delete context;
                continue;
            }

            if (context->wsa_buf.buf == reinterpret_cast<char*>(&context->header)) {
                // Получен заголовок - готовимся принять данные
                LoggerContext* new_context = new LoggerContext();
                new_context->wsa_buf.buf = new_context->buffer;
                new_context->wsa_buf.len = context->header.size;

                SocketIPC* channel = reinterpret_cast<SocketIPC*>(completion_key);
                channel->async_receive(new_context->buffer, context->header.size, &new_context->overlapped);

                delete context;
            }
            else {
                // Получены данные - пишем в файл
                self->log_file_ << context->buffer << std::endl;
                self->log_file_.flush();

                // Готовимся к следующему заголовку
                context->wsa_buf.buf = reinterpret_cast<char*>(&context->header);
                context->wsa_buf.len = sizeof(MessageHeader);

                SocketIPC* channel = reinterpret_cast<SocketIPC*>(completion_key);
                channel->async_receive(&context->header, sizeof(MessageHeader), &context->overlapped);
            }
        }
        return 0;
    }
};

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    DataLogger logger;
    logger.run();

    WSACleanup();
    return 0;
}