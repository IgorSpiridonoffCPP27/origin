#include "header.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

// Функция для выполнения HTTP-запроса
void do_http_request(net::io_context& ioc, const std::string& host, const std::string& port, const std::string& target) {
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    auto const results = resolver.resolve(host, port);
    stream.connect(results);

    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "Boost.Beast");
    req.set(http::field::accept, "text/html");

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    std::cout << "Status: " << res.result_int() << " " << res.reason() << "\n";
    std::cout << "Body:\n" << res.body() << "\n";

    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != beast::errc::not_connected) {
        throw beast::system_error{ec};
    }
}

// Функция для выполнения HTTPS-запроса
void do_https_request(net::io_context& ioc, ssl::context& ctx, const std::string& host, const std::string& port, const std::string& target) {
    tcp::resolver resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
    }

    auto const results = resolver.resolve(host, port);
    beast::get_lowest_layer(stream).connect(results);
    stream.handshake(ssl::stream_base::client);

    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "Boost.Beast");
    req.set(http::field::accept, "text/html");

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    std::cout << "Status: " << res.result_int() << " " << res.reason() << "\n";
    std::cout << "Body:\n" << res.body() << "\n";

    beast::error_code ec;
    stream.shutdown(ec);
    if (ec == net::error::eof || ec == ssl::error::stream_truncated) {
        ec = {};
    }
    if (ec) {
        throw beast::system_error{ec};
    }
}

int main() {
    setlocale(LC_ALL, "Rus");
    try {
        net::io_context ioc;
        
        // Запрашиваем URL у пользователя
        std::string url;
        std::cout << "Enter URL (e.g., http://example.com or https://example.com): ";
        std::getline(std::cin, url);

        // Парсим URL
        std::string protocol, host, port, target;
        
        size_t protocol_end = url.find("://");
        if (protocol_end != std::string::npos) {
            protocol = url.substr(0, protocol_end);
            size_t host_start = protocol_end + 3;
            size_t path_start = url.find('/', host_start);
            
            if (path_start != std::string::npos) {
                host = url.substr(host_start, path_start - host_start);
                target = url.substr(path_start);
            } else {
                host = url.substr(host_start);
                target = "/";
            }
            
            // Проверяем, есть ли порт в host
            size_t port_pos = host.find(':');
            if (port_pos != std::string::npos) {
                port = host.substr(port_pos + 1);
                host = host.substr(0, port_pos);
            } else {
                port = (protocol == "https") ? "443" : "80";
            }
        } else {
            std::cerr << "Invalid URL format. Please include http:// or https://\n";
            return 1;
        }

        if (protocol == "https") {
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_verify_mode(ssl::verify_none); // Отключаем проверку сертификата для простоты
            do_https_request(ioc, ctx, host, port, target);
        } else if (protocol == "http") {
            do_http_request(ioc, host, port, target);
        } else {
            std::cerr << "Unsupported protocol: " << protocol << "\n";
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        system("pause");
        return 1;
    }
    system("pause");
    return 0;
}