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

// Прототип функции
DownloadResult follow_redirects(const std::string& initial_url, int max_redirects = 5);

// Глобальные переменные для управления потоками
std::atomic<bool> keep_running(true);
std::mutex data_mutex;
std::set<int> processed_word_ids;

// Альтернатива std::wstring_convert (удален в C++17)
std::wstring utf8_to_wstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

void process_word(DBuse& db, const std::string& word) {
    std::cout << "\nПоиск по слову: " << word << std::endl;
    
    std::vector<std::string> test_urls = {
        "https://ru.wikipedia.org/wiki/" + word,
        "https://www.google.com/search?q=" + word,
        "https://www.britannica.com/search?query=" + word
    };

    bool found = false;
    int word_id = db.get_word_id(word);
    if (word_id == -1) {
        std::cerr << "Слово не найдено в базе данных" << std::endl;
        return;
    }

    for (const auto& url : test_urls) {
        std::cout << "Пробуем URL: " << url << std::endl;
        DownloadResult result = follow_redirects(url);
        
        if (!result.html.empty()) {
            std::cout << "Успешно скачано с конечного URL: " << result.final_url << std::endl;
            
            // Сохраняем URL в базу данных
            db.save_word_url(word_id, result.final_url, result.html);
            
            // Сохраняем также в файл (опционально)
            std::wstring wide_filename = utf8_to_wstring(word) + L"_page.html";
            std::ofstream out(wide_filename, std::ios::binary);
            out << result.html;
            std::cout << "HTML сохранен в " << word << "_page.html (" 
                      << result.html.size() << " байт)" << std::endl;
            
            found = true;
            break;
        }
    }
    
    if (!found) {
        std::cerr << "Не удалось получить результаты для слова: " << word << std::endl;
    }
}

void monitor_new_words(DBuse& db) {
    int last_processed_id = db.get_max_word_id();
    
    while (keep_running) {
        try {
            auto new_words = db.get_new_words_since(last_processed_id);
            
            if (!new_words.empty()) {
                std::lock_guard<std::mutex> lock(data_mutex);
                for (const auto& [word_id, word] : new_words) {
                    if (processed_word_ids.insert(word_id).second) {
                        process_word(db, word);
                    }
                }
                last_processed_id = db.get_max_word_id();
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
        } catch (const std::exception& e) {
            std::cerr << "Ошибка в потоке мониторинга: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}

DownloadResult follow_redirects(const std::string& initial_url, int max_redirects) {
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

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    try {
        DBuse db("localhost", "HTTP", "test_postgres", "12345678");
        db.create_tables();
        // Обрабатываем существующие слова
        auto initial_words = db.get_all_words();
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            int max_id = db.get_max_word_id();
            processed_word_ids.insert(max_id);
            
            for (const auto& word : initial_words) {
                process_word(db, word);
            }
        }
        
        // Запускаем асинхронный мониторинг новых слов
        std::thread monitor_thread(monitor_new_words, std::ref(db));
        
        // Главный поток может делать что-то еще или просто ждать
        std::cout << "Мониторинг новых слов запущен. Нажмите Enter для выхода...\n";
        std::cin.get();
        
        // Завершение работы
        keep_running = false;
        monitor_thread.join();
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        keep_running = false;
        system("pause");
        return 1;
    }
    
    return 0;
}