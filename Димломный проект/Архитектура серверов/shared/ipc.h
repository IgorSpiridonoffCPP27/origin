#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include "protocol.h"


#pragma comment(lib, "ws2_32.lib")

class IPCChannel {
public:
    virtual bool async_send(const void* data, size_t size, LPOVERLAPPED overlapped) = 0;
    virtual bool async_receive(void* buffer, size_t size, LPOVERLAPPED overlapped) = 0;
    virtual ~IPCChannel() = default;
};

class SocketIPC : public IPCChannel {
    SOCKET socket_;
    bool is_server_;

public:
    SocketIPC(const std::string& ip, int port, bool as_server = false);
    ~SocketIPC();

    bool async_send(const void* data, size_t size, LPOVERLAPPED overlapped) override;
    bool async_receive(void* buffer, size_t size, LPOVERLAPPED overlapped) override;

private:
    void connect_to_server(const std::string& ip, int port);
    void start_server(const std::string& ip, int port);
};