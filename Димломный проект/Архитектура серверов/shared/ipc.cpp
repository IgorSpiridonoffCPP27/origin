#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ipc.h"
#include <stdexcept>


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
        connect_to_server(ip, port);
    }
}

SocketIPC::~SocketIPC() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
    }
    WSACleanup();
}

bool SocketIPC::async_send(const void* data, size_t size, LPOVERLAPPED overlapped) {
    WSABUF wsa_buf;
    wsa_buf.buf = (CHAR*)data;
    wsa_buf.len = (ULONG)size;

    DWORD bytes_sent;
    return WSASend(socket_, &wsa_buf, 1, &bytes_sent, 0, overlapped, NULL) == 0;
}

bool SocketIPC::async_receive(void* buffer, size_t size, LPOVERLAPPED overlapped) {
    WSABUF wsa_buf;
    wsa_buf.buf = (CHAR*)buffer;
    wsa_buf.len = (ULONG)size;

    DWORD bytes_recv, flags = 0;
    return WSARecv(socket_, &wsa_buf, 1, &bytes_recv, &flags, overlapped, NULL) == 0;
}

void SocketIPC::connect_to_server(const std::string& ip, int port) {
    socket_ = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (socket_ == INVALID_SOCKET) {
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    InetPtonA(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (WSAConnect(socket_, (sockaddr*)&server_addr, sizeof(server_addr), NULL, NULL, NULL, NULL) == SOCKET_ERROR) {
        closesocket(socket_);
        throw std::runtime_error("Connection failed");
    }
}

void SocketIPC::start_server(const std::string& ip, int port) {
    SOCKET listen_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listen_socket == INVALID_SOCKET) {
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(listen_socket);
        throw std::runtime_error("Bind failed");
    }

    listen(listen_socket, SOMAXCONN);

    sockaddr_in client_addr{};
    int addr_len = sizeof(client_addr);
    socket_ = WSAAccept(listen_socket, (sockaddr*)&client_addr, &addr_len, NULL, NULL);
    closesocket(listen_socket);

    if (socket_ == INVALID_SOCKET) {
        throw std::runtime_error("Accept failed");
    }
}