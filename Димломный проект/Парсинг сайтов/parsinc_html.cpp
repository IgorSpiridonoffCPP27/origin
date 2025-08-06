#include <iostream>
#include <string>
#include <vector>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/regex.hpp>
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#pragma comment(lib, "shell32.lib")

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;


struct DownloadResult {
    std::string html;
    std::string final_url;
};

DownloadResult follow_redirects(const std::string& initial_url, int max_redirects = 5) {
    std::string current_url = initial_url;
    DownloadResult result;

    for (int i = 0; i < max_redirects; ++i) {
        try {
            // Парсинг URL
            size_t protocol_pos = current_url.find("://");
            if (protocol_pos == std::string::npos) {
                current_url = "https://" + current_url;
                protocol_pos = current_url.find("://");
            }

            std::string protocol = current_url.substr(0, protocol_pos);
            std::string host = current_url.substr(protocol_pos + 3);
            std::string port = (protocol == "https") ? "443" : "80";
            std::string target;

            size_t path_pos = host.find('/');
            if (path_pos != std::string::npos) {
                target = host.substr(path_pos);
                host = host.substr(0, path_pos);
            } else {
                target = "/";
            }

            // Удаляем порт из host если есть
            size_t colon_pos = host.find(':');
            if (colon_pos != std::string::npos) {
                port = host.substr(colon_pos + 1);
                host = host.substr(0, colon_pos);
            }

            // Создаем контекст
            net::io_context ioc;
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_default_verify_paths();
            ctx.set_verify_mode(ssl::verify_none);

            if (protocol == "https") {
                // SSL соединение
                beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
                
                if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
                    throw beast::system_error(
                        beast::error_code(static_cast<int>(::ERR_get_error()),
                        net::error::get_ssl_category()));
                }

                auto const results = net::ip::tcp::resolver(ioc).resolve(host, port);
                beast::get_lowest_layer(stream).connect(results);
                stream.handshake(ssl::stream_base::client);

                http::request<http::string_body> req{http::verb::get, target, 11};
                req.set(http::field::host, host);
                req.set(http::field::user_agent, "Mozilla/5.0");
                req.set(http::field::accept, "text/html");

                http::write(stream, req);

                beast::flat_buffer buffer;
                http::response<http::dynamic_body> res;
                http::read(stream, buffer, res);

                // Проверяем код ответа
                if (res.result() == http::status::moved_permanently || 
                    res.result() == http::status::found) {
                    // Получаем новый URL из Location
                    auto location = res.find(http::field::location);
                    if (location != res.end()) {
                        current_url = location->value();
                        continue; // Следуем по редиректу
                    }
                }

                // Если не редирект, сохраняем результат
                result.html = beast::buffers_to_string(res.body().data());
                result.final_url = current_url;
                break;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при обработке URL " << current_url << ": " << e.what() << std::endl;
            break;
        }
    }

    return result;
}


int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    std::string query;
    std::cout << "Введите поисковый запрос: ";
    std::getline(std::cin, query);

        // 2. Пытаемся получить конечную страницу через редиректы
    std::vector<std::string> test_urls = {
        "https://wikipedia.org/wiki/" + query,
        "https://www.britannica.com/search?query=" + query,
        "https://www.google.com/search?q=" + query
    };

    for (const auto& url : test_urls) {
        std::cout << "\nПробуем URL: " << url << std::endl;
        DownloadResult result = follow_redirects(url);
        
        if (!result.html.empty()) {
            std::cout << "Успешно скачано с конечного URL: " << result.final_url << std::endl;
            
            std::ofstream out("final_page.html");
            out << result.html;
            std::cout << "HTML сохранен в final_page.html (" << result.html.size() << " байт)" << std::endl;
            
            system("pause");
            return 0;
        }
    }

    std::cerr << "Не удалось получить конечную страницу" << std::endl;
    system("pause");
    return 1;
}