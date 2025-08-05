#include <iostream>
#include <string>
#include <vector>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include "DBusers.h"
#include <iomanip>
#include <sstream>
#include <cctype>
#include <clocale>
#include <locale>
#include <codecvt>
#include <thread>
#include <chrono>
#include <set>


#pragma comment(lib, "shell32.lib")

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;

// Структура для хранения результата
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

            size_t colon_pos = host.find(':');
            if (colon_pos != std::string::npos) {
                port = host.substr(colon_pos + 1);
                host = host.substr(0, colon_pos);
            }

            net::io_context ioc;
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_default_verify_paths();
            ctx.set_verify_mode(ssl::verify_none);

            if (protocol == "https") {
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

                if (res.result() == http::status::moved_permanently || 
                    res.result() == http::status::found) {
                    auto location = res.find(http::field::location);
                    if (location != res.end()) {
                        current_url = location->value();
                        continue;
                    }
                }

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

std::vector<std::string> get_words_from_database(DBuse& db) {
    std::vector<std::string> words;
    
    db.execute_in_transaction([&](pqxx::work& txn) {
        pqxx::result result = txn.exec("SELECT words FROM words ORDER BY id");
        
        for (auto row : result) {
            words.push_back(row["words"].as<std::string>());
        }
    });
    
    return words;
}


void process_word(const std::string& word) {
    std::cout << "\nПоиск по слову: " << word << std::endl;
    
    std::vector<std::string> test_urls = {
        "https://ru.wikipedia.org/wiki/" + word,
        "https://www.google.com/search?q=" + word,
        "https://www.britannica.com/search?query=" + word
    };

    bool found = false;
    for (const auto& url : test_urls) {
        std::cout << "Пробуем URL: " << url << std::endl;
        DownloadResult result = follow_redirects(url);
        
        if (!result.html.empty()) {
            std::cout << "Успешно скачано с конечного URL: " << result.final_url << std::endl;
            
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::wstring wide_filename = converter.from_bytes(word) + L"_page.html";
            std::ofstream out(wide_filename, std::ios::binary);
            out << result.html;
            std::cout << "HTML сохранен в " << word << "_page.html (" << result.html.size() << " байт)" << std::endl;
            
            found = true;
            break;
        }
    }
    
    if (!found) {
        std::cerr << "Не удалось получить результаты для слова: " << word << std::endl;
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    try {
        DBuse db("localhost", "HTTP", "test_postgres", "12345678");
        
        // Обрабатываем существующие слова
        std::vector<std::string> processed_words = get_words_from_database(db);
        std::set<std::string> unique_words(processed_words.begin(), processed_words.end());
        
        for (const auto& word : processed_words) {
            process_word(word);
        }
        
        // Бесконечный цикл проверки новых слов
        while (true) {
            std::cout << "\nПроверяем новые слова..." << std::endl;
            
            // Получаем текущий список слов из базы
            std::vector<std::string> current_words = get_words_from_database(db);
            
            // Находим новые слова
            for (const auto& word : current_words) {
                if (unique_words.find(word) == unique_words.end()) {
                    // Новое слово найдено
                    process_word(word);
                    unique_words.insert(word);
                }
            }
            
            // Пауза между проверками (например, 30 секунд)
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        system("pause");
        return 1;
    }
    
    return 0;
}