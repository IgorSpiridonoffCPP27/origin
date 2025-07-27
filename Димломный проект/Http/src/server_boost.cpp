#include "pch.h"
#include "utils.h"
#include "server_control.h"
#include "file_operations.h"
#include "DBusers.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

int main() {
    set_utf8_locale();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    DBuse connectiondb("localhost", "HTTP", "test_postgres", "12345678");
    connectiondb.create_tables();
    connectiondb.add_unique_constraint();

    std::cout << "Server started at http://localhost:8080\n";

    try {
        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {tcp::v4(), 8080}};
        
        std::thread control_thread(server_control_thread);

        while (server_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!server_running) break;

            tcp::socket socket(ioc);
            boost::system::error_code ec;
            
            acceptor.accept(socket, ec);
            if (ec && server_running) {
                std::cerr << "Accept error: " << ec.message() << "\n";
                continue;
            }

            std::thread([sock = std::move(socket), &connectiondb]() mutable {
                try {
                    beast::flat_buffer buffer;
                    http::request<http::string_body> req;
                    boost::system::error_code ec;

                    http::read(sock, buffer, req, ec);
                    if (ec && ec != http::error::end_of_stream) {
                        std::cerr << "Read error: " << ec.message() << "\n";
                        return;
                    }

                    http::response<http::string_body> res;
                    handle_request(req, res, connectiondb);
                    res.set(http::field::connection, "close");
                    http::write(sock, res, ec);
                    sock.shutdown(tcp::socket::shutdown_send, ec);
                } 
                catch (const std::exception& e) {
                    std::cerr << "Connection exception: " << e.what() << "\n";
                }
            }).detach();
        }

        acceptor.close();
        if (control_thread.joinable()) {
            control_thread.join();
        }
        
        // Если graceful_shutdown не вызвал exit, завершаем здесь
        if (!shutdown_complete) {
            std::cout << "\nPress Enter to exit...";
            std::cin.ignore();
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        graceful_shutdown("Emergency shutdown");
        return 1;
    }
    return 0;
}