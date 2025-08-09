#include "pch.h"
#include "DBusers.h"
#include <gumbo.h>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/ssl.hpp>
#include <atomic>
#include <queue>
#include <shared_mutex>
#include <fstream>
#include <csignal>
#include <openssl/crypto.h>
#include <boost/beast/core/detail/base64.hpp>
#include <iomanip>
#include <sstream>
#include <boost/tokenizer.hpp>

// Глобальный логгер
class Logger
{
public:
    static Logger &instance()
    {
        static Logger logger;
        return logger;
    }

    void log(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[LOG] " << message << std::endl;
        logfile_ << "[LOG] " << message << std::endl;
        logfile_.flush();
    }

private:
    Logger()
    {
        logfile_.open("crawler_log.txt", std::ios::app);
        logfile_ << "\n\n===== NEW SESSION =====" << std::endl;
        logfile_ << "Starting at: " << __TIMESTAMP__ << std::endl;
    }

    ~Logger()
    {
        logfile_ << "===== SESSION END =====" << std::endl;
        logfile_.close();
    }

    std::ofstream logfile_;
    std::mutex mutex_;
};

#define LOG(msg) Logger::instance().log(msg)

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

// Конфигурация краулера
struct CrawlerConfig
{
    int max_redirects = 5;
    int recursion_depth = 2;
    int max_connections = 10;
    int max_links_per_page = 3;
    std::chrono::seconds poll_interval{5};
};

class ThreadSafeSet
{
public:
    bool insert(const std::string &value)
    {
        std::unique_lock lock(mutex_);
        return visited_.insert(value).second;
    }

    bool contains(const std::string &value) const
    {
        std::shared_lock lock(mutex_);
        return visited_.find(value) != visited_.end();
    }

    void clear()
    {
        std::unique_lock lock(mutex_);
        visited_.clear();
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_set<std::string> visited_;
};

// Функция для URL-кодирования строк
std::string url_encode(const std::string &value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (auto c : value)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || uc == '-' || uc == '_' || uc == '.' || uc == '~')
        {
            escaped << c;
        }
        else
        {
            escaped << '%' << std::setw(2) << static_cast<int>(uc);
        }
    }

    return escaped.str();
}

class HtmlParser
{
public:
    static std::vector<std::string> extract_links(const std::string &html, const std::string &base_url)
    {
        std::vector<std::string> links;
        GumboOutput *output = gumbo_parse(html.c_str());
        if (!output)
        {
            LOG("Gumbo parse failed for HTML content");
            return links;
        }

        extract_links(output->root, base_url, links);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
        return links;
    }

    static std::string normalize_url(const std::string &url, const std::string &base_url)
    {
        if (url.empty() || url[0] == '#' || url.find("javascript:") == 0)
        {
            return "";
        }

        if (url.find("://") != std::string::npos)
        {
            return url;
        }

        size_t protocol_pos = base_url.find("://");
        if (protocol_pos == std::string::npos)
        {
            return "";
        }

        size_t domain_end = base_url.find('/', protocol_pos + 3);
        if (domain_end == std::string::npos)
        {
            domain_end = base_url.length();
        }

        if (url[0] == '/')
        {
            return base_url.substr(0, domain_end) + url;
        }
        else
        {
            size_t last_slash = base_url.rfind('/');
            if (last_slash != std::string::npos && last_slash >= protocol_pos + 3)
            {
                return base_url.substr(0, last_slash + 1) + url;
            }
        }
        return "";
    }

private:
    static void extract_links(GumboNode *node, const std::string &base_url, std::vector<std::string> &links)
    {
        if (node->type != GUMBO_NODE_ELEMENT)
            return;

        if (node->v.element.tag == GUMBO_TAG_A)
        {
            GumboAttribute *href = gumbo_get_attribute(&node->v.element.attributes, "href");
            if (href)
            {
                std::string url = normalize_url(href->value, base_url);
                if (!url.empty())
                {
                    links.push_back(url);
                }
            }
        }

        GumboVector *children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i)
        {
            extract_links(static_cast<GumboNode *>(children->data[i]), base_url, links);
        }
    }
};

class Crawler
{
public:
    Crawler(DBuse &db, const CrawlerConfig &config)
        : db_(db), config_(config),
          pool_(config.max_connections)
    {
        try
        {
            ssl_ctx_.set_default_verify_paths();
            ssl_ctx_.set_verify_mode(ssl::verify_none);
            LOG("SSL context initialized successfully (verify_none)");
        }
        catch (const std::exception &e)
        {
            LOG("SSL context error: " + std::string(e.what()));
            throw;
        }
    }

    ~Crawler()
    {
        stop();
    }

    void start()
    {
        LOG("Starting crawler");
        running_ = true;
        monitor_thread_ = std::thread([this]()
                                      {
            LOG("Monitor thread started");
            try {
                monitor_new_words();
            } catch (const std::exception& e) {
                LOG("Monitor thread exception: " + std::string(e.what()));
            }
            LOG("Monitor thread exiting"); });
    }

    void stop()
    {
        if (!running_)
            return;

        LOG("Stopping crawler...");
        running_ = false;

        if (monitor_thread_.joinable())
        {
            monitor_thread_.join();
            LOG("Monitor thread joined");
        }

        pool_.join();
        LOG("Thread pool joined");
        LOG("Crawler stopped");
    }

    net::thread_pool &get_pool() { return pool_; }

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
        return url_encode(result);
    }

    void process_word(const std::string &word)
    {
        LOG("Processing word: " + word);
        visited_urls_.clear();

        try
        {
            int word_id = db_.get_word_id(word);
            if (word_id == -1)
            {
                LOG("Word not found in database: " + word);
                return;
            }

            db_.execute_in_transaction([&](pqxx::work &txn)
                                       { txn.exec_params(
                                             "UPDATE words SET status = 'processing' WHERE id = $1",
                                             word_id); });

            std::vector<std::pair<std::string, std::string>> url_templates = {
                {"Wiktionary", "https://ru.wiktionary.org/wiki/" + prepare_wiki_url(word)},
                {"Wikipedia", "https://ru.wikipedia.org/wiki/" + prepare_wiki_url(word)}};

            bool processed = false;
            for (const auto &[site_name, url] : url_templates)
            {
                if (db_.url_exists_for_word(word_id, url))
                {
                    LOG("URL already processed: " + url);
                    continue;
                }

                LOG("Downloading: " + url);
                auto result = download_page(url);

                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                if (!result.ec && !result.html.empty())
                {
                    LOG("Download successful, size: " + std::to_string(result.html.size()));

                    // Обрабатываем базовую страницу и рекурсивно связанные
                    std::vector<std::pair<std::string, std::string>> base_page = {{result.final_url, result.html}};
                    process_recursive(word, site_name, base_page, 0);

                    processed = true;
                    break;
                }
                else
                {
                    LOG("Download failed for " + url + ": " + result.ec.message());
                }
            }

            db_.execute_in_transaction([&](pqxx::work &txn)
                                       { txn.exec_params(
                                             "UPDATE words SET status = 'completed', processed_at = CURRENT_TIMESTAMP WHERE id = $1",
                                             word_id); });

            if (!processed)
            {
                LOG("No valid URLs processed for word: " + word);
            }
        }
        catch (const std::exception &e)
        {
            LOG("Error processing word '" + word + "': " + e.what());
        }
        LOG("Finished processing word: " + word);
    }

private:
    DBuse &db_;
    CrawlerConfig config_;
    net::thread_pool pool_;
    ssl::context ssl_ctx_{ssl::context::tlsv12_client};
    std::atomic<bool> running_{true};
    std::thread monitor_thread_;
    ThreadSafeSet visited_urls_;

    struct DownloadResult
    {
        std::string html;
        std::string final_url;
        beast::error_code ec;
    };

    // Основная функция рекурсивной обработки
    void process_recursive(const std::string &word,
                           const std::string &site_name,
                           const std::vector<std::pair<std::string, std::string>> &pages,
                           int current_depth)
    {
        LOG("Processing at depth: " + std::to_string(current_depth) + " with " +
            std::to_string(pages.size()) + " pages");

        if (pages.empty())
            return;

        save_pages(word, site_name, pages, current_depth);

        if (current_depth >= config_.recursion_depth)
        {
            LOG("Max recursion depth reached: " + std::to_string(current_depth));
            return;
        }

        std::vector<std::pair<std::string, std::string>> next_level_pages;
        std::mutex next_level_mutex;
        std::vector<std::future<void>> futures;

        int word_id = db_.get_word_id(word);
        if (word_id == -1)
        {
            LOG("Word ID not found for: " + word);
            return;
        }

        std::string db_string = db_.get_full_word_string(word);
        if (db_string.empty())
            db_string = word;

        for (const auto &[url, html] : pages)
        {
            // Используем boost::asio::post вместо executor().post()
            futures.emplace_back(
                boost::asio::post(
                    pool_,
                    std::packaged_task<void()>([&, url, html]()
                                               {
                    auto links = HtmlParser::extract_links(html, url);
                    LOG("Found " + std::to_string(links.size()) + " links at: " + url);
                    
                    std::string current_host = get_host(url);
                    int links_processed = 0;
                    
                    for (const auto& link : links) {
                        if (links_processed >= config_.max_links_per_page) break;
                        
                        if (get_host(link) != current_host) {
                            LOG("Skipping external link: " + link);
                            continue;
                        }
                        
                        if (visited_urls_.contains(link)) {
                            LOG("Skipping visited URL: " + link);
                            continue;
                        }
                        
                        if (db_.url_exists_for_word(word_id, link)) {
                            LOG("URL already in DB: " + link);
                            continue;
                        }
                        
                        LOG("Downloading link: " + link);
                        auto result = download_page(link);
                        if (result.ec || result.html.empty()) {
                            LOG("Failed to download link: " + link);
                            continue;
                        }
                        
                        if (check_content(result.html, db_string)) {
                            LOG("Valid content found: " + link);
                            visited_urls_.insert(result.final_url);
                            
                            std::lock_guard lock(next_level_mutex);
                            next_level_pages.emplace_back(result.final_url, result.html);
                            links_processed++;
                        } else {
                            LOG("Content check failed: " + link);
                        }
                    } })));
        }

        for (auto &future : futures)
        {
            future.wait();
        }

        if (!next_level_pages.empty())
        {
            LOG("Found " + std::to_string(next_level_pages.size()) + " pages for next level");
            process_recursive(word, site_name, next_level_pages, current_depth + 1);
        }
    }

    // Сохраняет статистику для группы страниц
    void save_pages(const std::string &word,
                    const std::string &site_name,
                    const std::vector<std::pair<std::string, std::string>> &pages,
                    int depth)
    {
        LOG("Saving " + std::to_string(pages.size()) + " pages for word: " + word);

        int word_id = db_.get_word_id(word);
        if (word_id == -1)
        {
            LOG("Word ID not found for: " + word);
            return;
        }

        std::string db_string = db_.get_full_word_string(word);
        if (db_string.empty())
            db_string = word;

        try
        {
            db_.execute_in_transaction([&](pqxx::work &txn)
                                       {
                for (const auto& [url, html] : pages) {
                    std::string plain_text = extract_plain_text(html);
                    int count = count_word_occurrences(plain_text, db_string);
                    
                    // Используем существующий метод сохранения
                    db_.save_word_url(word_id, url, html, count);
                    LOG("Saved URL: " + url + " (count: " + std::to_string(count) + ")");
                } });
        }
        catch (const std::exception &e)
        {
            LOG("DB save error: " + std::string(e.what()));
        }
    }

    void monitor_new_words()
    {
        LOG("Starting word monitoring");
        int last_processed_id = db_.get_max_word_id();
        LOG("Last processed ID: " + std::to_string(last_processed_id));

        while (running_)
        {
            try
            {
                auto new_words = db_.get_new_words_since(last_processed_id);
                LOG("Found " + std::to_string(new_words.size()) + " new words");

                if (!new_words.empty())
                {
                    for (const auto &[word_id, word] : new_words)
                    {
                        LOG("Scheduling word: " + word);
                        net::post(pool_, [this, word]()
                                  {
                            LOG("Processing word in thread: " + word);
                            process_word(word); });
                    }
                    last_processed_id = db_.get_max_word_id();
                    LOG("Updated last processed ID: " + std::to_string(last_processed_id));
                }
                std::this_thread::sleep_for(config_.poll_interval);
            }
            catch (const std::exception &e)
            {
                LOG("Monitoring error: " + std::string(e.what()));
                std::this_thread::sleep_for(config_.poll_interval * 2);
            }
        }
        LOG("Monitoring loop exited");
    }

    DownloadResult download_page(const std::string &url, int redirect_count = 0)
    {
        LOG("Downloading: " + url + " (redirect: " + std::to_string(redirect_count) + ")");

        DownloadResult result;
        result.final_url = url;

        if (redirect_count > config_.max_redirects)
        {
            LOG("Too many redirects for URL: " + url);
            result.ec = boost::system::error_code(boost::system::errc::too_many_links,
                                                  boost::system::system_category());
            return result;
        }

        try
        {
            net::io_context ioc;
            tcp::resolver resolver(ioc);

            size_t protocol_pos = url.find("://");
            if (protocol_pos == std::string::npos)
            {
                throw std::runtime_error("Invalid URL format");
            }

            std::string protocol = url.substr(0, protocol_pos);
            std::string host_path = url.substr(protocol_pos + 3);
            size_t path_pos = host_path.find('/');
            std::string host = host_path.substr(0, path_pos);
            std::string path = (path_pos == std::string::npos) ? "/" : host_path.substr(path_pos);

            LOG("Resolving: " + host);
            auto const results = resolver.resolve(host, protocol);
            LOG("Resolved " + host);

            if (protocol == "https")
            {
                beast::ssl_stream<beast::tcp_stream> stream(ioc, ssl_ctx_);

                if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
                {
                    throw beast::system_error(
                        beast::error_code(static_cast<int>(::ERR_get_error()),
                                          net::error::get_ssl_category()));
                }

                LOG("Connecting to: " + host);
                beast::get_lowest_layer(stream).connect(results);
                LOG("SSL handshake...");
                stream.handshake(ssl::stream_base::client);

                http::request<http::string_body> req{http::verb::get, path, 11};
                req.set(http::field::host, host);
                req.set(http::field::user_agent, "Mozilla/5.0 (compatible; CrawlerBot/1.0)");
                req.set(http::field::accept, "text/html");

                LOG("Sending request to: " + url);
                http::write(stream, req);

                beast::flat_buffer buffer;
                http::response<http::dynamic_body> res;
                LOG("Reading response...");
                http::read(stream, buffer, res);
                LOG("Response status: " + std::to_string(res.result_int()));

                if (res.result() == http::status::moved_permanently ||
                    res.result() == http::status::found ||
                    res.result() == http::status::see_other)
                {

                    auto location = res.find(http::field::location);
                    if (location != res.end())
                    {
                        std::string new_url = HtmlParser::normalize_url(
                            std::string(location->value()), url);

                        if (!new_url.empty())
                        {
                            LOG("Redirecting to: " + new_url);
                            return download_page(new_url, redirect_count + 1);
                        }
                    }
                }

                result.html = beast::buffers_to_string(res.body().data());
                LOG("Downloaded " + std::to_string(result.html.size()) + " bytes");

                beast::error_code ec;
                stream.shutdown(ec);
                if (ec == net::error::eof)
                {
                    ec = {};
                }
                if (ec)
                {
                    LOG("SSL shutdown error: " + ec.message());
                }
            }
            else
            {
                beast::tcp_stream stream(ioc);
                LOG("Connecting to: " + host);
                stream.connect(results);

                http::request<http::string_body> req{http::verb::get, path, 11};
                req.set(http::field::host, host);
                req.set(http::field::user_agent, "Mozilla/5.0 (compatible; CrawlerBot/1.0)");
                req.set(http::field::accept, "text/html");

                LOG("Sending request to: " + url);
                http::write(stream, req);

                beast::flat_buffer buffer;
                http::response<http::dynamic_body> res;
                LOG("Reading response...");
                http::read(stream, buffer, res);
                LOG("Response status: " + std::to_string(res.result_int()));

                if (res.result() == http::status::moved_permanently ||
                    res.result() == http::status::found ||
                    res.result() == http::status::see_other)
                {

                    auto location = res.find(http::field::location);
                    if (location != res.end())
                    {
                        std::string new_url = HtmlParser::normalize_url(
                            std::string(location->value()), url);

                        if (!new_url.empty())
                        {
                            LOG("Redirecting to: " + new_url);
                            return download_page(new_url, redirect_count + 1);
                        }
                    }
                }

                result.html = beast::buffers_to_string(res.body().data());
                LOG("Downloaded " + std::to_string(result.html.size()) + " bytes");

                beast::error_code ec;
                stream.socket().shutdown(tcp::socket::shutdown_both, ec);
                if (ec == net::error::not_connected)
                {
                    ec = {};
                }
                if (ec)
                {
                    LOG("TCP shutdown error: " + ec.message());
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG("Download exception: " + std::string(e.what()) + " for URL: " + url);
            result.ec = boost::system::error_code(1, boost::system::system_category());
        }
        return result;
    }

    std::string get_host(const std::string &url)
    {
        size_t protocol_pos = url.find("://");
        if (protocol_pos == std::string::npos)
        {
            return "";
        }
        size_t host_start = protocol_pos + 3;
        size_t host_end = url.find('/', host_start);
        if (host_end == std::string::npos)
        {
            host_end = url.length();
        }
        return url.substr(host_start, host_end - host_start);
    }

    std::string extract_plain_text(const std::string &html)
    {
        GumboOutput *output = gumbo_parse(html.c_str());
        if (!output)
        {
            LOG("Gumbo parse failed for text extraction");
            return "";
        }

        std::string text = extract_text(output->root);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
        return text;
    }

    static std::string extract_text(GumboNode *node)
    {
        if (node->type == GUMBO_NODE_TEXT)
        {
            return std::string(node->v.text.text);
        }
        else if (node->type == GUMBO_NODE_ELEMENT)
        {
            if (node->v.element.tag == GUMBO_TAG_SCRIPT ||
                node->v.element.tag == GUMBO_TAG_STYLE)
            {
                return "";
            }

            std::string text;
            GumboVector *children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i)
            {
                text += extract_text(static_cast<GumboNode *>(children->data[i]));
            }
            return text;
        }
        return "";
    }

    int count_word_occurrences(const std::string &content, const std::string &words_str)
    {
        if (content.empty() || words_str.empty())
            return 0;

        int total_count = 0;
        std::string lower_content = boost::algorithm::to_lower_copy(content);

        // Разбиваем строку слов на отдельные слова
        boost::char_separator<char> sep(" ");
        boost::tokenizer<boost::char_separator<char>> tokens(words_str, sep);

        for (const auto &word : tokens)
        {
            if (word.empty())
                continue;

            std::string lower_word = boost::algorithm::to_lower_copy(word);
            int count = 0;
            size_t pos = 0;

            while ((pos = lower_content.find(lower_word, pos)) != std::string::npos)
            {
                count++;
                pos += lower_word.length();
            }

            total_count += count;
        }

        return total_count;
    }

    bool check_content(const std::string &content, const std::string &words_str)
    {
        if (words_str.empty())
            return false;

        std::string text = extract_plain_text(content);
        if (text.empty())
            return false;

        std::string lower_text = boost::algorithm::to_lower_copy(text);

        // Разбиваем строку слов на отдельные слова
        boost::char_separator<char> sep(" ");
        boost::tokenizer<boost::char_separator<char>> tokens(words_str, sep);

        for (const auto &word : tokens)
        {
            if (word.empty())
                continue;

            std::string lower_word = boost::algorithm::to_lower_copy(word);
            if (lower_text.find(lower_word) != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }
};

void signal_handler(int signal)
{
    LOG("Interrupt signal (" + std::to_string(signal) + ") received");
    exit(signal);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::set_terminate([]()
                       {
        LOG("Terminate called!");
        if (auto exc = std::current_exception()) {
            try {
                std::rethrow_exception(exc);
            } catch (const std::exception& e) {
                LOG("Uncaught exception: " + std::string(e.what()));
            } catch (...) {
                LOG("Unknown exception");
            }
        }
        LOG("Aborting...");
        std::abort(); });

    bool force_restart = false;
    if (argc > 1 && std::string(argv[1]) == "--force")
    {
        force_restart = true;
        LOG("Force restart mode enabled");
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try
    {
        LOG("Initializing database...");
        DBuse db("localhost", "HTTP", "test_postgres", "12345678");

        LOG("Creating tables...");
        db.create_tables();

        LOG("Initializing crawler...");
        CrawlerConfig config;
        Crawler crawler(db, config);

        LOG("Starting crawler...");
        crawler.start();

        auto initial_words = db.get_all_words();
        LOG("Found " + std::to_string(initial_words.size()) + " initial words");

        for (const auto &word : initial_words)
        {
            int word_id = db.get_word_id(word);
            if (word_id != -1)
            {
                std::string wiki_url = "https://ru.wiktionary.org/wiki/" + crawler.prepare_wiki_url(word);
                if (force_restart || !db.url_exists_for_word(word_id, wiki_url))
                {
                    LOG("Scheduling initial word: " + word);
                    net::post(crawler.get_pool(), [&crawler, word]()
                              { crawler.process_word(word); });
                }
                else
                {
                    LOG("Skipping already processed word: " + word);
                }
            }
        }

        LOG("Crawler started. Press Enter to exit...");
        std::cin.get();

        LOG("Stopping crawler...");
        crawler.stop();
        LOG("Crawler stopped");
    }
    catch (const std::exception &e)
    {
        LOG("Fatal error: " + std::string(e.what()));
        return 1;
    }
    catch (...)
    {
        LOG("Unknown fatal error");
        return 2;
    }

    LOG("Application exited normally");
    return 0;
}