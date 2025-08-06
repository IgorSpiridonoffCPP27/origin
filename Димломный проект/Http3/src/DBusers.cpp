#include "DBusers.h"
#include <iostream>
#include <atomic>
#include <mutex>
DBuse::DBuse(const std::string &host, const std::string &dbname,
             const std::string &user, const std::string &password) : conn("host=" + host + " dbname=" + dbname +
                                                                          " user=" + user + " password=" + password +
                                                                          " client_encoding=UTF8")
{
    if (!conn.is_open())
    {
        throw std::runtime_error("Не удалось подключиться к БД");
    }
    std::cout << "Успешное подключение к " << conn.dbname() << std::endl;
}

void DBuse::execute_in_transaction(const std::function<void(pqxx::work &)> &func)
{
    pqxx::work txn(conn);
    try
    {
        func(txn);
        txn.commit();
    }
    catch (...)
    {
        txn.abort();
        throw;
    }
}

json::json DBuse::get_tables()
{
    json::json result;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        pqxx::result tables = txn.exec("SELECT table_name FROM information_schema.tables WHERE table_schema='public'");
        for (auto row : tables) result.push_back(row[0].as<std::string>()); });
    return result;
}

json::json DBuse::get_columns(const std::string &table)
{
    json::json result;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        pqxx::result cols = txn.exec_params(
            "SELECT column_name, data_type FROM information_schema.columns "
            "WHERE table_name=$1 AND (column_default IS NULL OR column_default NOT LIKE 'nextval%') "
            "ORDER BY ordinal_position", 
            table
        );
        for (auto row : cols) {
            result.push_back({
                {"name", row[0].as<std::string>()},
                {"type", row[1].as<std::string>()}
            });
        } });
    return result;
}

json::json DBuse::get_record(const std::string &table,
                             const std::string &column,
                             const std::string &value)
{
    json::json record;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        pqxx::result result = txn.exec_params(
            "SELECT * FROM " + txn.quote_name(table) + 
            " WHERE " + txn.quote_name(column) + " = " + txn.quote(value)
        );
        
        if (result.empty()) {
            throw std::runtime_error("Record not found");
        }
        
        for (const auto& field : result[0]) {
            record[field.name()] = field.as<std::string>();
        } });
    return record;
}

bool DBuse::check_auth(const std::string &username,
                       const std::string &password)
{
    bool auth_result = false;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        auto res = txn.exec_params(
            "SELECT 1 FROM users WHERE username=$1 AND password=$2", 
            username, password
        );
        auth_result = !res.empty(); });
    return auth_result;
}

void DBuse::check_column_exists(pqxx::work &txn, const std::string &table, const std::string &column)
{
    auto res = txn.exec_params(
        "SELECT 1 FROM information_schema.columns "
        "WHERE table_name=$1 AND column_name=$2",
        table, column);
    if (res.empty())
    {
        throw std::runtime_error("Столбец " + column + " не существует в таблице " + table);
    }
}

void DBuse::insert_data(const std::string &table,
                        const json::json &values,
                        json::json &response)
{
    try
    {
        execute_in_transaction([&](pqxx::work &txn)
                               {
            // Проверяем существование всех колонок
            for (const auto& [name, value] : values.items()) {
                check_column_exists(txn, table, name);
            }

            // Проверяем дубликаты перед вставкой
            for (const auto& [name, value] : values.items()) {
                std::string val = value.is_string() ? value.get<std::string>() : value.dump();
                
                auto type_res = txn.exec_params(
                    "SELECT data_type FROM information_schema.columns "
                    "WHERE table_name=$1 AND column_name=$2",
                    table, name
                );
                
                if (!type_res.empty()) {
                    std::string type = type_res[0][0].as<std::string>();
                    if (type.find("char") != std::string::npos || type == "text") {
                        auto check = txn.exec("SELECT * FROM " + txn.quote_name(table) + 
                                   " WHERE " + txn.quote_name(name) + " = " + txn.quote(val));
                        if (!check.empty()) {
                            json::json existing_record;
                            for (const auto& field : check[0]) {
                                if (field.name() != "id") { // Исключаем поле id
                                    existing_record[field.name()] = field.is_null() ? nullptr : field.as<std::string>();
                                }
                            }
                            
                            response = {
                                {"status", 409},
                                {"message", "Запись с таким значением уже существует"},
                                {"duplicate_field", name},
                                {"duplicate_value", val},
                                {"existing_record", existing_record}
                            };
                            return;
                        }
                    }
                }
            }

            // Если дубликатов нет, выполняем вставку
            std::string query = "INSERT INTO " + txn.quote_name(table) + " (";
            std::string values_part = ") VALUES (";
            bool first = true;

            for (const auto& [name, value] : values.items()) {
                if (!first) {
                    query += ", ";
                    values_part += ", ";
                }
                first = false;
                
                query += txn.quote_name(name);
                std::string val = value.is_string() ? value.get<std::string>() : value.dump();
                values_part += txn.quote(val);
            }

            query += values_part + ") RETURNING *";
            
            auto result = txn.exec(query);
            if (!result.empty()) {
                json::json new_record;
                for (const auto& field : result[0]) {
                    if (field.name() != "id") { // Исключаем поле id
                        new_record[field.name()] = field.is_null() ? nullptr : field.as<std::string>();
                    }
                }
                
                response = {
                    {"status", 201},
                    {"message", "Данные успешно добавлены"},
                    {"new_record", new_record}
                };
            } });
    }
    catch (const std::exception &e)
    {
        response = {
            {"status", 500},
            {"message", std::string("Ошибка сервера: ") + e.what()}};
    }
}

void DBuse::create_tables()
{
    execute_in_transaction([&](pqxx::work &txn)
                           {
        txn.exec(
            "CREATE TABLE IF NOT EXISTS words("
            "id SERIAL PRIMARY KEY, "
            "word VARCHAR(100) NOT NULL UNIQUE)"
        );
        txn.exec(
            "CREATE TABLE IF NOT EXISTS word_urls("
            "id SERIAL PRIMARY KEY, "
            "word_id INTEGER REFERENCES words(id) ON DELETE CASCADE, "
            "url VARCHAR(500) NOT NULL, "
            "html_content TEXT, "
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
            "UNIQUE(word_id, url))"  // Добавляем уникальный constraint
        );

        std::cout << "Таблица words и word_urls успешно создана\n"; });
}

bool DBuse::save_word_url(int word_id, const std::string &url, const std::string &html_content)
{
    bool saved = false;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        // Проверяем, существует ли уже такой URL для этого слова
        auto exists = txn.exec_params(
            "SELECT 1 FROM word_urls WHERE word_id = $1 AND url = $2 LIMIT 1",
            word_id, url
        );
        
        if (exists.empty()) {
            txn.exec_params(
                "INSERT INTO word_urls (word_id, url, html_content) "
                "VALUES ($1, $2, $3)",
                word_id, url, html_content
            );
            saved = true;
        } });
    return saved;
}

int DBuse::get_word_id(const std::string &word)
{
    int word_id = -1;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        auto result = txn.exec_params(
            "SELECT id FROM words WHERE word = $1", 
            word
        );
        if (!result.empty()) {
            word_id = result[0][0].as<int>();
        } });
    return word_id;
}

bool DBuse::url_exists_for_word(int word_id, const std::string &url)
{
    bool exists = false;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        auto result = txn.exec_params(
            "SELECT 1 FROM word_urls WHERE word_id = $1 AND url = $2 LIMIT 1",
            word_id, url
        );
        exists = !result.empty(); });
    return exists;
}

void DBuse::add_unique_constraint()
{
    // Теперь UNIQUE constraint добавляется сразу при создании таблицы
    std::cout << "UNIQUE constraint уже установлен при создании таблицы\n";
}

void DBuse::add_word_to_tables(const std::string &word)
{
    if (word.empty())
    {
        std::cerr << "Пустое слово не может быть добавлено\n";
        return;
    }

    if (word.length() > 100)
    {
        std::cerr << "Слово '" << word << "' слишком длинное (макс. 100 символов)\n";
        return;
    }

    execute_in_transaction([&](pqxx::work &txn)
                           {
        try {
            auto result = txn.exec_params(
                "INSERT INTO words (word) VALUES ($1) "
                "ON CONFLICT (word) DO UPDATE SET word=EXCLUDED.word "
                "RETURNING id",
                word
            );

            if (!result.empty()) {
                const int word_id = result[0]["id"].as<int>();
                std::cout << "Слово '" << word << "' обработано. ID: " << word_id << "\n";
            }
        } catch (const pqxx::unique_violation& e) {
            // Если слово уже существует, получаем его ID
            auto existing = txn.exec_params(
                "SELECT id FROM words WHERE word = $1", 
                word
            );
            if (!existing.empty()) {
                const int word_id = existing[0]["id"].as<int>();
                std::cout << "Слово '" << word << "' уже существует. ID: " << word_id << "\n";
            }
        } });
}

std::vector<std::string> DBuse::get_all_words()
{
    std::vector<std::string> words;
    execute_in_transaction([&](pqxx::work &txn)
                           {
            auto result = txn.exec("SELECT word FROM words ORDER BY id");
            for (auto row : result) {
                words.push_back(row["word"].as<std::string>());
            } });
    return words;
}

std::vector<std::pair<int, std::string>> DBuse::get_new_words_since(int last_id)
{
    std::vector<std::pair<int, std::string>> new_words;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        auto result = txn.exec_params(
            "SELECT id, word FROM words WHERE id > $1 ORDER BY id", 
            last_id
        );
        for (auto row : result) {
            new_words.emplace_back(
                row["id"].as<int>(),
                row["word"].as<std::string>()
            );
        } });
    return new_words;
}

int DBuse::get_max_word_id()
{
    int max_id = 0;
    execute_in_transaction([&](pqxx::work &txn)
                           {
            auto result = txn.exec("SELECT COALESCE(MAX(id), 0) FROM words");
            max_id = result[0][0].as<int>(); });
    return max_id;
}


