#pragma once
#include "pch.h"
#include <memory>
#include <atomic>
#include <mutex>
#include <functional>

namespace json = nlohmann;

class DBuse {
public:
    DBuse(const std::string& host, 
          const std::string& dbname,
          const std::string& user,
          const std::string& password);
    
    // Transaction handling
    void execute_in_transaction(const std::function<void(pqxx::work&)>& func);
    
    // Metadata operations
    json::json get_tables();
    json::json get_columns(const std::string& table);
    
    // Authentication
    bool check_auth(const std::string& username, 
                   const std::string& password);
    
    // Data operations
    void insert_data(const std::string& table, 
                    const json::json& values, 
                    json::json& response);
    
    // Word processing operations
    json::json get_record(const std::string& table,
                         const std::string& column,
                         const std::string& value);
    json::json process_word_request(const std::string& word);
    json::json check_word_status(int word_id);
    bool save_word_url(int word_id,
                          const std::string &url,
                          const std::string &html_content,
                          const std::string &text_content,
                          int word_count);
    
    // Utility methods
    void create_tables();
    void add_unique_constraint();
    void add_word_to_tables(const std::string& word);
    int get_word_id(const std::string& word);
    bool url_exists_for_word(int word_id, const std::string& url);
    std::string get_full_word_string(const std::string& word);
    std::vector<std::string> get_all_words();
    std::vector<std::pair<int, std::string>> get_new_words_since(int last_id);
    int get_max_word_id();
    void run_migrations();
private:
    struct ConnectionDetails {
        std::string host;
        std::string dbname;
        std::string user;
        std::string password;
    };
    struct ConnectionWrapper {
        std::unique_ptr<pqxx::connection> conn;
        std::atomic<bool> in_use{false};
    };
    ConnectionDetails conn_details;
    std::vector<std::shared_ptr<ConnectionWrapper>> connection_pool;
    mutable std::mutex pool_mutex;
    std::condition_variable pool_cv;

    std::shared_ptr<ConnectionWrapper> get_connection();
    void return_connection(std::shared_ptr<ConnectionWrapper> wrapper);
    void initialize_connection(std::shared_ptr<ConnectionWrapper> wrapper);
    void check_column_exists(pqxx::work& txn, 
                            const std::string& table, 
                            const std::string& column);
};