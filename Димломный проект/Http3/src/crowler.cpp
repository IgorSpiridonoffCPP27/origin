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
#include <boost/regex.hpp> // Добавляем в начале файла

#pragma comment(lib, "shell32.lib")

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;

// Структура для хранения результата
struct DownloadResult
{
    std::string html;
    std::string final_url;
};

// Прототип функции
DownloadResult follow_redirects(const std::string &initial_url, int max_redirects = 5);

// Глобальные переменные для управления потоками
std::atomic<bool> keep_running(true);
std::mutex data_mutex;
std::set<int> processed_word_ids;

// Альтернатива std::wstring_convert (удален в C++17)
std::wstring utf8_to_wstring(const std::string &str)
{
std::wstring utf8_to_wstring(const std::string &str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}
std::string prepare_wiki_url(const std::string &word)
{
    std::string result;
    for (char c : word)
    {
        if (c == ' ')
        {
            result += '_';
        }
        else
        {
            result += c;
        }
    }
    return result;
}

std::vector<std::string> extract_links_from_html(const std::string &html_content, const std::string &base_url)
{
    std::vector<std::string> links;

    try
    {
        // Регулярное выражение для поиска ссылок в HTML
        static const boost::regex link_regex(
            "<a\\s+[^>]*href\\s*=\\s*[\"']([^\"']+)[\"'][^>]*>",
            boost::regex::icase | boost::regex::optimize);

        boost::smatch matches;
        std::string::const_iterator start = html_content.begin();
        std::string::const_iterator end = html_content.end();

        while (boost::regex_search(start, end, matches, link_regex))
        {
            std::string link = matches[1].str();

            // Пропускаем якорные ссылки и javascript
            if (!link.empty() && link[0] != '#' &&
                link.find("javascript:") == std::string::npos)
            {

                // Если ссылка относительная, преобразуем в абсолютную
                if (link.find("://") == std::string::npos)
                {
                    if (link[0] == '/')
                    {
                        // Абсолютный путь на том же домене
                        size_t protocol_pos = base_url.find("://");
                        size_t domain_end = base_url.find('/', protocol_pos + 3);
                        if (domain_end == std::string::npos)
                        {
                            domain_end = base_url.length();
                        }
                        link = base_url.substr(0, domain_end) + link;
                    }
                    else
                    {
                        // Относительный путь
                        size_t last_slash = base_url.rfind('/');
                        if (last_slash != std::string::npos)
                        {
                            link = base_url.substr(0, last_slash + 1) + link;
                        }
                    }
                }

                links.push_back(link);
            }

            start = matches[0].second;
        }
    }
    catch (const boost::regex_error &e)
    {
        std::cerr << "Ошибка при парсинге HTML: " << e.what() << std::endl;
    }

    return links;
}

void save_links_recursive(const std::string& word, const std::string& site_name, 
                         const std::string& url, const std::string& html_content,
                         int recursion_depth, int current_depth = 0) {
    // Извлекаем все ссылки из HTML
    std::vector<std::string> all_links = extract_links_from_html(html_content, url);
    std::vector<std::string> valid_links;

    // Фильтруем ссылки, оставляя только те, где есть искомое слово
    for (const auto& link : all_links) {
        // Пропускаем внешние и якорные ссылки сразу
        if (link.find(url.substr(0, url.find("://") + 3)) == std::string::npos || 
            link.find('#') != std::string::npos) {
            continue;
        }

        try {
            std::cout << "Проверяем ссылку: " << link << std::endl;
            DownloadResult nested_result = follow_redirects(link);
            
            if (!nested_result.html.empty() && 
                nested_result.html.find(word) != std::string::npos) {
                valid_links.push_back(link);
                std::cout << "  ✓ Слово найдено, добавляем ссылку" << std::endl;
            } else {
                std::cout << "  ✗ Слово не найдено, пропускаем" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Ошибка при проверке ссылки: " << e.what() << std::endl;
        }
    }

    // Сохраняем ТОЛЬКО валидные ссылки в файл
    if (!valid_links.empty()) {
        std::wstring links_filename = utf8_to_wstring(word) + L"_" + 
                                    utf8_to_wstring(site_name) + 
                                    (current_depth > 0 ? L"_level" + std::to_wstring(current_depth) : L"") + 
                                    L"_links.txt";
        
        std::ofstream links_out(links_filename);
        links_out << "Валидные ссылки из: " << url << "\n";
        links_out << "Глубина: " << current_depth << "\n";
        links_out << "Найдено ссылок: " << valid_links.size() << "\n\n";

        for (const auto& link : valid_links) {
            links_out << link << "\n";
        }

        std::wcout << L"Сохранено " << valid_links.size() << L" валидных ссылок в: " << links_filename << std::endl;
    }

    // Рекурсивная обработка (если нужно идти глубже)
    if (current_depth < recursion_depth && !valid_links.empty()) {
        const size_t max_links_to_follow = 5; // Ограничиваем глубину рекурсии
        size_t processed = 0;

        for (const auto& link : valid_links) {
            if (processed++ >= max_links_to_follow) break;

            try {
                DownloadResult nested_result = follow_redirects(link);
                if (!nested_result.html.empty()) {
                    save_links_recursive(word, site_name, nested_result.final_url, 
                                      nested_result.html, recursion_depth, 
                                      current_depth + 1);
                }
            } catch (const std::exception& e) {
                std::cerr << "Ошибка рекурсивной обработки: " << e.what() << std::endl;
            }
        }
    }
}

void process_word(DBuse &db, const std::string &word,int recursion_depth=1)
{
    std::cout << "\nПоиск по слову: " << word << std::endl;

    int word_id = db.get_word_id(word);
    if (word_id == -1)
    {
        std::cerr << "Слово не найдено в базе данных" << std::endl;
        return;
    }

    // Список URL для попыток (в порядке приоритета)
    std::vector<std::pair<std::string, std::string>> url_templates = {
        {"Wiktionary", "https://ru.wiktionary.org/wiki/" + prepare_wiki_url(word)},
        {"Wikipedia", "https://ru.wikipedia.org/wiki/" + prepare_wiki_url(word)}};

    bool found = false;

    for (const auto &[site_name, url] : url_templates)
    {
        std::cout << "Пробуем " << site_name << " URL: " << url << std::endl;
        try
        {
            DownloadResult result = follow_redirects(url);

            if (!result.html.empty() && result.html.find("400 Bad request") == std::string::npos)
            {
                std::cout << "Успешно скачано с " << site_name
                          << " (" << result.final_url << ")" << std::endl;

                if (db.save_word_url(word_id, result.final_url, result.html))
                {
                    std::cout << "URL сохранен в базу данных" << std::endl;

                    // Сохраняем HTML в файл
                    std::wstring wide_filename = utf8_to_wstring(word) + L"_" + utf8_to_wstring(site_name) + L".html";
                    std::ofstream out(wide_filename, std::ios::binary);
                    out << result.html;
                    std::cout << "HTML сохранен в файл: " << word << "_" << site_name << ".html" << std::endl;
                    
                }
                else
                {
                    std::cout << "URL уже существует в базе данных" << std::endl;
                }

////////////////////////////////////////
// Используем рекурсивную функцию для сохранения ссылок
                    save_links_recursive(word, site_name, result.final_url, 
                                       result.html, recursion_depth,1);
                
////////////////////////////////////////


                found = true;
                break;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Ошибка при запросе к " << site_name << ": " << e.what() << std::endl;
        }
    }

    if (!found)
    {
        std::cerr << "Не удалось получить результаты для слова: " << word << std::endl;
    }
}
void monitor_new_words(DBuse &db)
{
    int last_processed_id = db.get_max_word_id();

    while (keep_running)
    {
        try
        {
            auto new_words = db.get_new_words_since(last_processed_id);

            if (!new_words.empty())
            {
                std::lock_guard<std::mutex> lock(data_mutex);
                for (const auto &[word_id, word] : new_words)
                {
                    if (processed_word_ids.insert(word_id).second)
                    {
                        process_word(db, word);
                    }
                }
                last_processed_id = db.get_max_word_id();
            }

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Ошибка в потоке мониторинга: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}

DownloadResult follow_redirects(const std::string &initial_url, int max_redirects)
{
    std::string current_url = initial_url;
    DownloadResult result;

    for (int i = 0; i < max_redirects; ++i)
    {
        try
        {
            // Парсинг URL
            size_t protocol_pos = current_url.find("://");
            if (protocol_pos == std::string::npos)
            {
                current_url = "https://" + current_url;
                protocol_pos = current_url.find("://");
            }

            std::string protocol = current_url.substr(0, protocol_pos);
            std::string host = current_url.substr(protocol_pos + 3);
            std::string port = (protocol == "https") ? "443" : "80";
            std::string target;

            size_t path_pos = host.find('/');
            if (path_pos != std::string::npos)
            {
                target = host.substr(path_pos);
                host = host.substr(0, path_pos);
            }
            else
            {
                target = "/";
            }

            size_t colon_pos = host.find(':');
            if (colon_pos != std::string::npos)
            {
                port = host.substr(colon_pos + 1);
                host = host.substr(0, colon_pos);
            }

            net::io_context ioc;
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_default_verify_paths();
            ctx.set_verify_mode(ssl::verify_none);

            if (protocol == "https")
            {
                beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

                if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
                {
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
                if (res.result() == http::status::bad_request)
                {
                    throw std::runtime_error("400 Bad Request");
                }
                if (res.result() == http::status::moved_permanently ||
                    res.result() == http::status::found)
                {
                    auto location = res.find(http::field::location);
                    if (location != res.end())
                    {
                        current_url = location->value();
                        continue;
                    }
                }

                result.html = beast::buffers_to_string(res.body().data());
                result.final_url = current_url;
                break;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Ошибка при обработке URL " << current_url << ": " << e.what() << std::endl;
            result.html.clear();
            break;
        }
    }

    return result;
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try
    {
        DBuse db("localhost", "HTTP", "test_postgres", "12345678");
        db.create_tables();
        // Обрабатываем существующие слова
        auto initial_words = db.get_all_words();
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            int max_id = db.get_max_word_id();
            processed_word_ids.insert(max_id);

            for (const auto &word : initial_words)
            {
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
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        keep_running = false;
        system("pause");
        return 1;
    }

    return 0;
}