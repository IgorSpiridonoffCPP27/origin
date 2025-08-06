#pragma once
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace json = nlohmann;

class DBuse {
public:
    DBuse(const std::string& host, const std::string& dbname, 
          const std::string& user, const std::string& password);
    
    // Методы для работы с таблицами
    json::json get_tables();
    json::json get_columns(const std::string& table);
    json::json get_record(const std::string& table, 
                         const std::string& column, 
                         const std::string& value);
    
    // Методы аутентификации
    bool check_auth(const std::string& username, 
                   const std::string& password);
    
    // Методы для работы с данными
    void insert_data(const std::string& table, 
                    const json::json& values, 
                    json::json& response);
    
    // Специфичные методы для работы со словами
    void create_tables();
    void add_unique_constraint();
    void add_word_to_tables(const std::string& word);
    void execute_in_transaction(const std::function<void(pqxx::work&)>& func);
    std::vector<std::pair<int, std::string>> get_new_words_since(int last_id);
    int get_max_word_id();
    std::vector<std::string> get_all_words();
    int get_word_id(const std::string& word);
    bool url_exists_for_word(int word_id, const std::string& url);
    bool save_word_url(int word_id, const std::string& url, const std::string& html_content);
    std::string get_full_word_string(const std::string &word);
private:
    pqxx::connection conn;
    void check_column_exists(pqxx::work& txn, const std::string& table, const std::string& column);
};