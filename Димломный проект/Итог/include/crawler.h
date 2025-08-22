#pragma once
#include "DBusers.h"
#include "crawler_config.h"
#include "threadsafe_set.h"
#include "html_parser.h"
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/ssl.hpp>
#include <atomic>
#include <queue>
#include <csignal>
#include <openssl/crypto.h>
#include <boost/tokenizer.hpp>
#include <unordered_map>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class Crawler {
public:
    Crawler(DBuse& db, const CrawlerConfig& config);
    ~Crawler();
    void start();
    void stop();

    std::string prepare_wiki_url(const std::string& word);
    void process_word(const std::string& word);

    // New crawling mode: start from a URL and index all words on pages
    void crawl_url(const std::string& url);
    void crawl_start_url();
    void update_config(const CrawlerConfig& new_config);

private:
    struct DownloadResult {
        std::string html;
        std::string final_url;
        beast::error_code ec;
    };

    DBuse& db_;
    CrawlerConfig config_;
    net::thread_pool pool_;
    ssl::context ssl_ctx_;
    std::atomic<bool> running_;
    std::thread monitor_thread_;
    ThreadSafeSet visited_urls_;

    void process_recursive(const std::string& word, const std::string& site_name,
                          const std::vector<std::pair<std::string, std::string>>& pages,
                          int current_depth);
    void save_pages(const std::string& word, const std::string& site_name,
                   const std::vector<std::pair<std::string, std::string>>& pages,
                   int depth);
    void monitor_new_words();
    DownloadResult download_page(const std::string& url, int redirect_count = 0);
    int count_word_occurrences(const std::string& content, const std::string& words_str);
    bool check_content(const std::string& content, const std::string& words_str);

    // Helpers for URL-first crawling mode
    void crawl_recursive(const std::vector<std::pair<std::string, std::string>>& pages,
                         int current_depth);
    void save_pages_all_words(const std::vector<std::pair<std::string, std::string>>& pages);
    std::unordered_map<std::string, int> compute_word_counts(const std::string& text);
    static bool is_word_char(unsigned char c);
};