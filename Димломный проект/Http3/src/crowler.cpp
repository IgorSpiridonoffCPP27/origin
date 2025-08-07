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
#include <queue>
#include <condition_variable>
#include <future>
#include <condition_variable>
#include <unordered_set>
#include <boost/regex.hpp> // Добавляем в начале файла
#include <boost/algorithm/string.hpp>
#pragma comment(lib, "shell32.lib")

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;

// Добавляем глобальный контейнер для хранения посещенных URL
std::mutex visited_urls_mutex;
std::unordered_set<std::string> visited_urls;

// Модифицированный ThreadPool с поддержкой возвращаемых значений
class ThreadPool {
public:
    explicit ThreadPool(size_t threads) : stop(false) {
        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this]{ return stop || !tasks.empty(); });
                        if(stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if(stop) throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace([task](){ (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread &worker: workers)
            worker.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};


// Новая структура для хранения статистики по словам
struct WordStats
{
    std::string word;
    int count;
};

// Структура для хранения результата
struct DownloadResult
{
    std::string html;
    std::string final_url;
};

// Прототип функции
DownloadResult follow_redirects(const std::string &initial_url, int max_redirects = 5);
std::string normalize_string(const std::string &str);
// Глобальные переменные для управления потоками
std::atomic<bool> keep_running(true);
std::mutex data_mutex;
std::set<int> processed_word_ids;

std::wstring utf8_to_wstring(const std::string &str)
{
    if (str.empty())
        return L"";
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

// Функция для подсчета вхождений слов
std::vector<WordStats> count_word_occurrences(const std::string &content, const std::string &search_words)
{
    std::vector<WordStats> stats;
    std::string normalized_content = normalize_string(content);
    std::string normalized_words = normalize_string(search_words);

    std::istringstream iss(normalized_words);
    std::vector<std::string> words_to_count((std::istream_iterator<std::string>(iss)),
                                            std::istream_iterator<std::string>());

    for (const auto &word : words_to_count)
    {
        int count = 0;
        size_t pos = 0;
        while ((pos = normalized_content.find(word, pos)) != std::string::npos)
        {
            count++;
            pos += word.length();
        }
        stats.push_back({word, count});
    }

    return stats;
}

// Модифицированная функция для сохранения ссылок с подсчетом слов
void save_links_with_counts(DBuse &db,
                            const std::string &word,
                            const std::string &site_name,
                            const std::string &base_url,
                            const std::string &base_html,
                            const std::vector<std::pair<std::string, std::string>> &links_content,
                            int current_depth)
{
    // Получаем строку из базы данных
    std::string db_string = db.get_full_word_string(word);
    if (db_string.empty())
    {
        db_string = word;
    }

    // Создаем имя файла
    std::wstring filename = utf8_to_wstring(word) + L"_" +
                            utf8_to_wstring(site_name) +
                            (current_depth > 0 ? L"_level" + std::to_wstring(current_depth) : L"") +
                            L"_links.txt";

    // Открываем файл для записи
    std::ofstream out(filename);
    out << "Базовая страница: " << base_url << "\n\n";

    // Подсчет для базовой страницы
    auto base_stats = count_word_occurrences(base_html, db_string);
    out << "Статистика для базовой страницы:\n";
    int base_total = 0;
    for (const auto &ws : base_stats)
    {
        out << "  " << ws.word << ": " << ws.count << "\n";
        base_total += ws.count;
    }
    out << "  Всего: " << base_total << "\n\n";

    out << "Найдено " << links_content.size() << " релевантных страниц:\n\n";

    // Обрабатываем каждую страницу
    for (const auto &[url, html] : links_content)
    {
        // Подсчитываем вхождения слов
        auto stats = count_word_occurrences(html, db_string);

        // Записываем URL и статистику
        out << "URL: " << url << "\n";
        out << "Количество вхождений:\n";

        int total = 0;
        for (const auto &ws : stats)
        {
            out << "  " << ws.word << ": " << ws.count << "\n";
            total += ws.count;
        }

        out << "  Всего: " << total << "\n\n";
    }

    std::wcout << L"Файл со статистикой сохранен: " << filename << std::endl;
}

std::vector<std::string> extract_links_from_html(const std::string &html_content, const std::string &base_url)
{
    std::vector<std::string> links;
    if (html_content.empty())
        return links;
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
    catch (const std::exception &e)
    {
        std::cerr << "Error extracting links: " << e.what() << std::endl;
    }

    return links;
}
// Функция для нормализации строки (приведение к нижнему регистру, удаление лишних пробелов)
std::string normalize_string(const std::string &str)
{
    if (str.empty())
        return "";
    try
    {
        // 1. Извлекаем содержимое между тегами <body> и </body>
        static const boost::regex body_regex("<body[^>]*>(.*?)</body>", boost::regex::icase);
        boost::smatch matches;
        std::string body_content;

        if (boost::regex_search(str, matches, body_regex))
        {
            body_content = matches[1].str();
        }
        else
        {
            // Если теги body не найдены, работаем со всем содержимым
            body_content = str;
        }

        // 2. Удаляем все HTML-теги
        static const boost::regex html_tags("<[^>]*>");
        std::string text = boost::regex_replace(body_content, html_tags, " ");

        // 3. Заменяем HTML-сущности на соответствующие символы
        static const std::vector<std::pair<boost::regex, std::string>> html_entities = {
            {boost::regex("&nbsp;"), " "},
            {boost::regex("&amp;"), "&"},
            {boost::regex("&lt;"), "<"},
            {boost::regex("&gt;"), ">"}};

        for (const auto &[regex, replacement] : html_entities)
        {
            text = boost::regex_replace(text, regex, replacement);
        }

        // 4. Удаляем множественные пробелы
        static const boost::regex multiple_spaces("\\s+");
        text = boost::regex_replace(text, multiple_spaces, " ");

        // 5. Приводим к нижнему регистру и обрезаем пробелы
        boost::algorithm::to_lower(text);
        boost::algorithm::trim(text);
        // Удаляем содержимое <script> тегов
        static const boost::regex script_regex("<script[^>]*>.*?</script>", boost::regex::icase);
        text = boost::regex_replace(text, script_regex, " ");

        // Удаляем содержимое <style> тегов
        static const boost::regex style_regex("<style[^>]*>.*?</style>", boost::regex::icase);
        text = boost::regex_replace(text, style_regex, " ");
        return text;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in normalize_string: " << e.what() << std::endl;
        return "";
    }
}

// Функция проверки содержимого страницы на соответствие словам из базы
bool check_content(const std::string &content, const std::string &db_string)
{
    // Нормализуем строку из базы и разбиваем на отдельные слова
    std::string normalized_db = normalize_string(db_string);
    std::istringstream iss(normalized_db);
    std::vector<std::string> db_words((std::istream_iterator<std::string>(iss)),
                                      std::istream_iterator<std::string>());

    // Нормализуем содержимое страницы
    std::string normalized_content = normalize_string(content);

    // Проверяем наличие хотя бы одного слова из базы в содержимом
    for (const auto &word : db_words)
    {
        if (normalized_content.find(word) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}
// Модифицированная рекурсивная функция с улучшенным логированием
void save_links_recursive(DBuse &db, const std::string &word, const std::string &site_name,
                         const std::string &url, const std::string &html_content,
                         int recursion_depth, ThreadPool& pool,
                         int current_depth = 0, const std::string &parent_path = "") {
    
    std::string current_path = parent_path.empty() ? "1" : parent_path + "." + std::to_string(current_depth);
    std::cout << "Проверяю " << current_path << " URL: " << url << std::endl;

    std::string db_string = db.get_full_word_string(word);
    if (db_string.empty()) db_string = word;

    auto all_links = extract_links_from_html(html_content, url);
    std::cout << "Total links found: " << all_links.size() << std::endl;
    
    std::vector<std::pair<std::string, std::string>> valid_links;
    std::mutex valid_links_mutex;
    std::vector<std::future<void>> futures;

    for (size_t i = 0; i < all_links.size(); ++i) {
        const auto &link = all_links[i];
        std::string link_path = current_path + "." + std::to_string(i + 1);

        if (link.find(url.substr(0, url.find("://") + 3)) == std::string::npos ||
            link.find('#') != std::string::npos) {
            std::cout << "  " << link_path << " Пропускаем внешнюю/якорную ссылку: " << link << std::endl;
            continue;
        }

        futures.emplace_back(
            pool.enqueue([&, link, link_path]() {
                try {
                    std::cout << "  " << link_path << " Проверяю ссылку: " << link << std::endl;
                    
                    {
                        std::lock_guard<std::mutex> lock(visited_urls_mutex);
                        if (visited_urls.count(link)) {
                            std::cout << "  " << link_path << " URL уже проверялся, пропускаем: " << link << std::endl;
                            return;
                        }
                    }

                    DownloadResult nested_result = follow_redirects(link);

                    if (!nested_result.html.empty() && check_content(nested_result.html, db_string)) {
                        {
                            std::lock_guard<std::mutex> lock(visited_urls_mutex);
                            visited_urls.insert(nested_result.final_url);
                        }
                        
                        {
                            std::lock_guard<std::mutex> lock(valid_links_mutex);
                            valid_links.emplace_back(nested_result.final_url, nested_result.html);
                        }
                        
                        std::cout << "    " << link_path << " ✓ Найдены совпадения\n";
                    } else {
                        std::cout << "    " << link_path << " ✗ Не содержит нужных слов\n";
                    }
                } catch (const std::exception &e) {
                    std::cerr << "  " << link_path << " Ошибка при проверке ссылки: " << e.what() << std::endl;
                }
            })
        );
    }

    // Ждем завершения всех задач
    for (auto &future : futures) {
        future.wait();
    }

    if (!valid_links.empty()) {
        save_links_with_counts(db, word, site_name, url, html_content, valid_links, current_depth);
        
        if (current_depth < recursion_depth) {
            for (const auto &[link, html] : valid_links) {
                save_links_recursive(db, word, site_name, link, html,
                                   recursion_depth, pool, current_depth + 1, current_path);
            }
        }
    }
}

// Модифицированная функция process_word
void process_word(DBuse &db, const std::string &word, int recursion_depth = 2) {
    std::cout << "\nПоиск по слову: " << word << std::endl;
    
    {
        std::lock_guard<std::mutex> lock(visited_urls_mutex);
        visited_urls.clear();
    }

    int word_id = db.get_word_id(word);
    if (word_id == -1) {
        std::cerr << "Слово не найдено в базе данных" << std::endl;
        return;
    }

    ThreadPool pool(4); // Создаем пул из 4 потоков

    std::vector<std::pair<std::string, std::string>> url_templates = {
        {"Wiktionary", "https://ru.wiktionary.org/wiki/" + prepare_wiki_url(word)},
        {"Wikipedia", "https://ru.wikipedia.org/wiki/" + prepare_wiki_url(word)}};

    for (const auto &[site_name, url] : url_templates) {
        try {
            DownloadResult result = follow_redirects(url);
            if (!result.html.empty()) {
                // Сохраняем HTML и обрабатываем ссылки
                save_links_recursive(db, word, site_name, result.final_url,
                                   result.html, recursion_depth, pool);
                return;
            }
        } catch (const std::exception &e) {
            std::cerr << "Ошибка при обработке " << site_name << ": " << e.what() << std::endl;
        }
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
    if (initial_url.empty())
        return result;
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
            std::cerr << "Error following redirects for " << initial_url << ": " << e.what() << std::endl;
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