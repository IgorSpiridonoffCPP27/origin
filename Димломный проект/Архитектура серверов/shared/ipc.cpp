#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ipc.h"
#include <stdexcept>
#include <iostream>

SocketIPC::SocketIPC(const std::string& ip, int port, bool as_server)
    : socket_(INVALID_SOCKET), is_server_(as_server) {

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }

    if (as_server) {
        start_server(ip, port);
    }
    else {
        int attempts = 3;
        while (attempts-- > 0) {
            try {
                connect_to_server(ip, port);
                return;
            }
            catch (const std::exception& e) {
                if (attempts == 0) throw;
                std::cerr << "Connection attempt failed (" << attempts << " left): " << e.what() << "\n";
                Sleep(1000);
            }
        }
    }
}

SocketIPC::~SocketIPC() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
    }
    WSACleanup();
}

bool SocketIPC::async_send(const void* data, size_t size, LPOVERLAPPED overlapped) {
    if (socket_ == INVALID_SOCKET) return false;

    WSABUF wsa_buf;
    wsa_buf.buf = (CHAR*)data;
    wsa_buf.len = (ULONG)size;

    DWORD bytes_sent;
    int result = WSASend(socket_, &wsa_buf, 1, &bytes_sent, 0, overlapped, NULL);

    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            std::cerr << "WSASend failed: " << error << std::endl;
            return false;
        }
    }
    return true;
}

bool SocketIPC::async_receive(void* buffer, size_t size, LPOVERLAPPED overlapped) {
    if (socket_ == INVALID_SOCKET) return false;

    WSABUF wsa_buf;
    wsa_buf.buf = (CHAR*)buffer;
    wsa_buf.len = (ULONG)size;

    DWORD bytes_recv, flags = 0;
    int result = WSARecv(socket_, &wsa_buf, 1, &bytes_recv, &flags, overlapped, NULL);

    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            std::cerr << "WSARecv failed: " << error << std::endl;
            return false;
        }
    }
    return true;
}

void SocketIPC::connect_to_server(const std::string& ip, int port) {
    socket_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (socket_ == INVALID_SOCKET) {
        throw std::runtime_error("Socket creation failed: " + std::to_string(WSAGetLastError()));
    }

    // Установка таймаутов
    DWORD timeout = 3000;
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1) {
        closesocket(socket_);
        throw std::runtime_error("Invalid address: " + ip);
    }

    if (WSAConnect(socket_, (sockaddr*)&server_addr, sizeof(server_addr),
        NULL, NULL, NULL, NULL) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        closesocket(socket_);
        throw std::runtime_error("Connection failed: " + std::to_string(error));
    }
}

void SocketIPC::start_server(const std::string& ip, int port) {
    SOCKET listen_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listen_socket == INVALID_SOCKET) {
        throw std::runtime_error("Socket creation failed: " + std::to_string(WSAGetLastError()));
    }

    // Разрешаем повторное использование адреса
    int optval = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
        closesocket(listen_socket);
        throw std::runtime_error("setsockopt failed: " + std::to_string(WSAGetLastError()));
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1) {
        closesocket(listen_socket);
        throw std::runtime_error("Invalid address: " + ip);
    }

    if (bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        closesocket(listen_socket);
        throw std::runtime_error("Bind failed: " + std::to_string(error));
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        closesocket(listen_socket);
        throw std::runtime_error("Listen failed: " + std::to_string(error));
    }

    // Основной цикл принятия подключений
    while (true) {
        sockaddr_in client_addr{};
        int addr_len = sizeof(client_addr);
        socket_ = WSAAccept(listen_socket, (sockaddr*)&client_addr, &addr_len, NULL, NULL);

        if (socket_ == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
            continue;
        }

        std::cout << "New client connected to logger: "
            << inet_ntoa(client_addr.sin_addr) << ":"
            << ntohs(client_addr.sin_port) << "\n";
        break;
    }
    closesocket(listen_socket);
}