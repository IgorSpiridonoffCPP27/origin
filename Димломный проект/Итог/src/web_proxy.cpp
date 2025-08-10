#include "pch.h"
#include "proxy_config.h"
#include "html_generator.h"
#include "api_client.h"
#include <boost/asio/ssl.hpp>
#include <atomic>
#include <thread>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

// Глобальный флаг для контроля работы прокси
std::atomic<bool> proxy_running{true};

// Обработчик сигналов для корректного завершения
void handle_signals(const boost::system::error_code& error, int signal) {
    if (!error && (signal == SIGINT || signal == SIGTERM)) {
        proxy_running = false;
    }
}

// Обработчик запросов
template<typename Stream>
void handle_request(
    Stream& stream,
    const std::string& api_host,
    const std::string& api_port,
    beast::flat_buffer& buffer)
{
    try {
        http::request<http::string_body> req;
        http::read(stream, buffer, req);

        http::response<http::string_body> res;

        if (req.method() == http::verb::get && req.target() == "/") {
            res.set(http::field::content_type, "text/html; charset=utf-8");
            res.body() = generate_html();
            res.prepare_payload();
        }
        else if (req.target().starts_with("/api")) {
            std::string target = req.target().substr(4);
            auto api_res = call_api(api_host, api_port, target, req.method(), req.body());
            
            res.result(api_res.result());
            for (const auto& field : api_res.base()) {
                res.set(field.name(), field.value());
            }
            res.body() = api_res.body();
            res.prepare_payload();
        }
        else {
            res.result(http::status::not_found);
            res.set(http::field::content_type, "application/json");
            res.body() = nlohmann::json{
                {"error", "Not Found"},
                {"path", std::string(req.target())}
            }.dump();
            res.prepare_payload();
        }

        http::write(stream, res);
    } catch(const beast::system_error& e) {
        if(e.code() != http::error::end_of_stream) {
            std::cerr << "Request handling error: " << e.what() << std::endl;
        }
    }
}

// Шаблонная функция для обработки сессии
template<typename Stream>
void do_session(
    Stream& stream,
    const std::string& api_host,
    const std::string& api_port,
    beast::flat_buffer& buffer)
{
    try {
        handle_request(stream, api_host, api_port, buffer);
        beast::error_code ec;
        // Исправление: правильное завершение соединения
        if constexpr (std::is_same_v<Stream, ssl::stream<tcp::socket>>) {
            // Для SSL: сначала shutdown SSL, затем TCP
            stream.shutdown(ec);
            if (ec == net::error::eof) {
                ec = {};
            }
            stream.next_layer().shutdown(tcp::socket::shutdown_send, ec);
        } else {
            // Для обычного TCP
            stream.shutdown(tcp::socket::shutdown_send, ec);
        }
    } catch(const std::exception& e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
}

// HTTP обработчик соединений
void run_http_proxy(
    net::io_context& ioc,
    const ProxyConfig& config)
{
    tcp::acceptor acceptor(ioc, {tcp::v4(), config.http_port});
    std::cout << "HTTP proxy listening on port " << config.http_port << std::endl;
    
    beast::flat_buffer buffer;
    
    while (proxy_running) {
        beast::error_code ec;
        tcp::socket socket(ioc);
        acceptor.accept(socket, ec);
        
        if (ec) {
            if (proxy_running) {
                std::cerr << "HTTP accept error: " << ec.message() << std::endl;
            }
            continue;
        }
        
        do_session(socket, config.api_host, config.api_port, buffer);
    }
}

// HTTPS обработчик соединений
void run_https_proxy(
    net::io_context& ioc,
    const ProxyConfig& config)
{
    try {
        ssl::context ctx{ssl::context::tlsv12_server};
        ctx.set_options(
            ssl::context::default_workarounds |
            ssl::context::no_sslv2 |
            ssl::context::single_dh_use);
        
        ctx.use_certificate_chain_file(config.cert_file);
        ctx.use_private_key_file(config.key_file, ssl::context::pem);
        
        tcp::acceptor acceptor(ioc, {tcp::v4(), config.https_port});
        std::cout << "HTTPS proxy listening on port " << config.https_port << std::endl;
        
        beast::flat_buffer buffer;
        
        while (proxy_running) {
            beast::error_code ec;
            tcp::socket socket(ioc);
            acceptor.accept(socket, ec);
            
            if (ec) {
                if (proxy_running) {
                    std::cerr << "HTTPS accept error: " << ec.message() << std::endl;
                }
                continue;
            }
            
            auto ssl_stream = std::make_unique<ssl::stream<tcp::socket>>(std::move(socket), ctx);
            ssl_stream->handshake(ssl::stream_base::server, ec);
            
            if (ec) {
                std::cerr << "SSL handshake error: " << ec.message() << std::endl;
                continue;
            }
            
            do_session(*ssl_stream, config.api_host, config.api_port, buffer);
        }
    } catch (const std::exception& e) {
        std::cerr << "HTTPS proxy error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        // Настройка консоли для поддержки UTF-8
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        // Конфигурация прокси
        ProxyConfig config;
        
        // Обработка сигналов для graceful shutdown
        net::io_context signal_ioc;
        net::signal_set signals(signal_ioc, SIGINT, SIGTERM);
        signals.async_wait(handle_signals);
        std::thread signal_thread([&signal_ioc]() { signal_ioc.run(); });
        
        // Запуск HTTP прокси
        std::thread http_thread([&]() {
            net::io_context ioc;
            run_http_proxy(ioc, config);
        });
        
        // Запуск HTTPS прокси
        std::thread https_thread([&]() {
            net::io_context ioc;
            run_https_proxy(ioc, config);
        });
        
        std::cout << "Web proxy started. Press Ctrl+C to exit." << std::endl;
        
        // Ожидание завершения работы
        http_thread.join();
        https_thread.join();
        
        // Остановка обработки сигналов
        signal_ioc.stop();
        signal_thread.join();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal proxy error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}