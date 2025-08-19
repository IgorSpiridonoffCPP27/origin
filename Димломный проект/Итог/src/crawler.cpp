#include "crawler.h"
#include "logger.h"
#include "url_tools.h"
#include <boost/beast/core/detail/base64.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string.hpp> // string split, tokenizer
#include <boost/tokenizer.hpp>
Crawler::Crawler(DBuse &db, const CrawlerConfig &config)
    : db_(db), config_(config), pool_(config.max_connections), ssl_ctx_(ssl::context::tlsv12_client)
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
// Safe ASCII-only lowercase to avoid CRT ctype assertions on UTF-8 bytes
static std::string ascii_lower_copy(const std::string& s)
{
    std::string r;
    r.reserve(s.size());
    for (unsigned char uc : s) {
        if (uc >= 'A' && uc <= 'Z') r.push_back(static_cast<char>(uc + 32));
        else r.push_back(static_cast<char>(uc));
    }
    return r;
}


// Реализации методов Crawler (process_recursive, save_pages, monitor_new_words, download_page, count_word_occurrences, check_content)
// [Здесь должна быть полная реализация методов класса Crawler из исходного кода]

Crawler::~Crawler()
{
    stop();
}

void Crawler::start()
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

void Crawler::stop()
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

net::thread_pool &Crawler::get_pool() { return pool_; }

std::string Crawler::prepare_wiki_url(const std::string &word)
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

void Crawler::process_word(const std::string &word)
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
            {"Wiktionary", config_.get("Crowler.start_page") + prepare_wiki_url(word)},
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

void Crawler::crawl_start_url()
{
    visited_urls_.clear();
    try {
        std::string start_url = config_.get("Crowler.start_page");
        if (start_url.empty()) {
            LOG("Start URL is empty in config");
            return;
        }
        auto result = download_page(start_url);
        if (!result.ec && !result.html.empty()) {
            std::vector<std::pair<std::string, std::string>> pages = {{result.final_url, result.html}};
            crawl_recursive(pages, 0);
        } else {
            LOG("Failed to download start URL: " + start_url);
        }
    } catch (const std::exception& e) {
        LOG(std::string("crawl_start_url exception: ") + e.what());
    }
}

void Crawler::crawl_url(const std::string &url)
{
    visited_urls_.clear();
    try {
        auto result = download_page(url);
        if (!result.ec && !result.html.empty()) {
            std::vector<std::pair<std::string, std::string>> pages = {{result.final_url, result.html}};
            crawl_recursive(pages, 0);
        } else {
            LOG("Failed to download URL: " + url);
        }
    } catch (const std::exception& e) {
        LOG(std::string("crawl_url exception: ") + e.what());
    }
}

void Crawler::process_recursive(const std::string &word,
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
void Crawler::save_pages(const std::string &word,
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
                    std::string plain_text = HtmlParser::extract_plain_text(html);
                    int count = count_word_occurrences(plain_text, db_string);
                    
                    // Используем существующий метод сохранения
                    db_.save_word_url(word_id, url, html, plain_text, count);
                    LOG("Saved URL: " + url + " (count: " + std::to_string(count) + ")");
                } });
    }
    catch (const std::exception &e)
    {
        LOG("DB save error: " + std::string(e.what()));
    }
}

// New: crawl recursively without targeting a specific word; index all words
void Crawler::crawl_recursive(const std::vector<std::pair<std::string, std::string>> &pages,
                              int current_depth)
{
    LOG("Crawl (all words) at depth: " + std::to_string(current_depth) +
        " with " + std::to_string(pages.size()) + " pages");

    if (pages.empty()) return;

    save_pages_all_words(pages);

    if (current_depth >= config_.recursion_depth) {
        LOG("Max recursion depth reached (all words): " + std::to_string(current_depth));
        return;
    }

    std::vector<std::pair<std::string, std::string>> next_level_pages;
    std::mutex next_level_mutex;
    std::vector<std::future<void>> futures;

    for (const auto &[url, html] : pages) {
        futures.emplace_back(
            boost::asio::post(
                pool_,
                std::packaged_task<void()>([&, url, html]() {
                    auto links = HtmlParser::extract_links(html, url);
                    LOG("[ALL] Found " + std::to_string(links.size()) + " links at: " + url);

                    std::string current_host = get_host(url);
                    int links_processed = 0;

                    for (const auto &link : links) {
                        if (links_processed >= config_.max_links_per_page) break;

                        if (get_host(link) != current_host) {
                            LOG("[ALL] Skipping external link: " + link);
                            continue;
                        }

                        if (visited_urls_.contains(link)) {
                            LOG("[ALL] Skipping visited URL: " + link);
                            continue;
                        }

                        LOG("[ALL] Downloading link: " + link);
                        auto result = download_page(link);
                        if (result.ec || result.html.empty()) {
                            LOG("[ALL] Failed to download link: " + link);
                            continue;
                        }

                        visited_urls_.insert(result.final_url);

                        std::lock_guard lock(next_level_mutex);
                        next_level_pages.emplace_back(result.final_url, result.html);
                        links_processed++;
                    }
                }))
        );
    }

    for (auto &future : futures) {
        future.wait();
    }

    if (!next_level_pages.empty()) {
        LOG("[ALL] Found " + std::to_string(next_level_pages.size()) + " pages for next level");
        crawl_recursive(next_level_pages, current_depth + 1);
    }
}

std::unordered_map<std::string, int> Crawler::compute_word_counts(const std::string &text)
{
    std::unordered_map<std::string, int> counts;
    if (text.empty()) return counts;

    // Treat common punctuation and whitespace as separators; keep UTF-8 letters intact
    static const std::string seps = " \t\n\r.,;:!?()[]{}<>\"'`~@#$%^&*-_=+\\/|\u00A0";
    boost::char_separator<char> sep(seps.c_str());
    auto lowered = ascii_lower_copy(text);
    boost::tokenizer<boost::char_separator<char>> tokens(lowered, sep);
    for (const auto &tok : tokens) {
        if (tok.size() < 2) continue; // skip 1-char tokens
        counts[tok] += 1;
    }
    return counts;
}

bool Crawler::is_word_char(unsigned char c)
{
    return std::isalnum(c) != 0;
}

void Crawler::save_pages_all_words(const std::vector<std::pair<std::string, std::string>> &pages)
{
    LOG("[ALL] Saving " + std::to_string(pages.size()) + " pages (all words)");
    try {
        for (const auto &pair : pages) {
            const std::string &url = pair.first;
            const std::string &html = pair.second;
            std::string plain_text = HtmlParser::extract_plain_text(html);
            auto counts = compute_word_counts(plain_text);

            for (const auto &entry : counts) {
                const std::string &word = entry.first;
                int count = entry.second;

                // Ensure word exists in words table, then save relation
                db_.add_word_to_tables(word);
                int word_id = db_.get_word_id(word);
                if (word_id == -1) continue;
                db_.save_word_url(word_id, url, html, plain_text, count);
            }
            LOG("[ALL] Indexed page: " + url + " (" + std::to_string(counts.size()) + " words)");
        }
    } catch (const std::exception &e) {
        LOG(std::string("save_pages_all_words error: ") + e.what());
    }
}

void Crawler::monitor_new_words()
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

Crawler::DownloadResult Crawler::download_page(const std::string &url, int redirect_count)
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
        // Кодируем цель HTTP: оставляем разделители, кодируем не-ASCII
        std::string http_target = encode_http_target(path);

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
            // Таймауты на сетевые операции
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
            beast::get_lowest_layer(stream).connect(results);
            LOG("SSL handshake...");
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
            stream.handshake(ssl::stream_base::client);

            http::request<http::string_body> req{http::verb::get, http_target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0 Safari/537.36");
            req.set(http::field::accept, "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
            req.set(http::field::accept_language, "ru,en-US;q=0.9,en;q=0.8");
            req.set(http::field::accept_encoding, "identity");
            req.set(http::field::connection, "close");

            LOG("Sending request to: " + url);
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
            http::write(stream, req);

            beast::flat_buffer buffer;
            LOG("Reading response...");
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
            http::response_parser<http::dynamic_body> parser;
            parser.body_limit(50ull * 1024ull * 1024ull); // 50 MB
            http::read(stream, buffer, parser);
            auto res = parser.release();
            LOG("Response status: " + std::to_string(res.result_int()));

            if (res.result() == http::status::moved_permanently ||
                res.result() == http::status::found ||
                res.result() == http::status::see_other ||
                res.result() == http::status::temporary_redirect ||
                res.result() == http::status::permanent_redirect)
            {
                auto location = res.find(http::field::location);
                if (location != res.end())
                {
                    std::string new_url = normalize_url(
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
            // Уберем жесткий shutdown, чтобы не зависать на серверах с некорректным TLS-выходом
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
            // Таймауты на сетевые операции
            stream.expires_after(std::chrono::seconds(15));
            stream.connect(results);

            http::request<http::string_body> req{http::verb::get, http_target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0 Safari/537.36");
            req.set(http::field::accept, "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
            req.set(http::field::accept_language, "ru,en-US;q=0.9,en;q=0.8");
            req.set(http::field::accept_encoding, "identity");
            req.set(http::field::connection, "close");

            LOG("Sending request to: " + url);
            stream.expires_after(std::chrono::seconds(15));
            http::write(stream, req);

            beast::flat_buffer buffer;
            LOG("Reading response...");
            stream.expires_after(std::chrono::seconds(30));
            http::response_parser<http::dynamic_body> parser;
            parser.body_limit(50ull * 1024ull * 1024ull); // 50 MB
            http::read(stream, buffer, parser);
            auto res = parser.release();
            LOG("Response status: " + std::to_string(res.result_int()));

            if (res.result() == http::status::moved_permanently ||
                res.result() == http::status::found ||
                res.result() == http::status::see_other ||
                res.result() == http::status::temporary_redirect ||
                res.result() == http::status::permanent_redirect)
            {
                auto location = res.find(http::field::location);
                if (location != res.end())
                {
                    std::string new_url = normalize_url(
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


int Crawler::count_word_occurrences(const std::string &content, const std::string &words_str)
{
    if (content.empty() || words_str.empty())
        return 0;

    int total_count = 0;
    std::string lower_content = ascii_lower_copy(content);

    // Разбиваем строку слов на отдельные слова
    boost::char_separator<char> sep(" ");
    boost::tokenizer<boost::char_separator<char>> tokens(words_str, sep);

    for (const auto &word : tokens)
    {
        if (word.empty())
            continue;

        std::string lower_word = ascii_lower_copy(word);
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

bool Crawler::check_content(const std::string &content, const std::string &words_str)
{
    if (words_str.empty())
        return false;

    std::string text = HtmlParser::extract_plain_text(content);
    if (text.empty())
        return false;

    std::string lower_text = ascii_lower_copy(text);

    // Разбиваем строку слов на отдельные слова
    boost::char_separator<char> sep(" ");
    boost::tokenizer<boost::char_separator<char>> tokens(words_str, sep);

    for (const auto &word : tokens)
    {
        if (word.empty())
            continue;

        std::string lower_word = ascii_lower_copy(word);
        if (lower_text.find(lower_word) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}