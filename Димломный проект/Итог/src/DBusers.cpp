#include "DBusers.h"
#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include "word_tools.h"

// Простая очистка до валидного UTF-8: пропускаем недопустимые байты/последовательности
static inline bool is_cont_byte(unsigned char b) { return (b & 0xC0) == 0x80; }

static std::string sanitize_utf8(const std::string &input)
{
    std::string out;
    out.reserve(input.size());
    const unsigned char *p = reinterpret_cast<const unsigned char *>(input.data());
    size_t i = 0;
    const size_t n = input.size();
    while (i < n)
    {
        unsigned char c = p[i];
        if (c <= 0x7F)
        {
            // ASCII
            out.push_back(static_cast<char>(c));
            ++i;
        }
        else if (c >= 0xC2 && c <= 0xDF)
        {
            // 2-byte
            if (i + 1 < n && is_cont_byte(p[i + 1]))
            {
                out.append(reinterpret_cast<const char *>(p + i), 2);
                i += 2;
            }
            else
            {
                // пропускаем некорректный стартовый байт
                ++i;
            }
        }
        else if (c == 0xE0)
        {
            // 3-byte, спец-диапазон для предотвращения overlong
            if (i + 2 < n && p[i + 1] >= 0xA0 && p[i + 1] <= 0xBF && is_cont_byte(p[i + 2]))
            {
                out.append(reinterpret_cast<const char *>(p + i), 3);
                i += 3;
            }
            else
            {
                ++i;
            }
        }
        else if (c >= 0xE1 && c <= 0xEF)
        {
            // 3-byte
            if (i + 2 < n && is_cont_byte(p[i + 1]) && is_cont_byte(p[i + 2]))
            {
                out.append(reinterpret_cast<const char *>(p + i), 3);
                i += 3;
            }
            else
            {
                ++i;
            }
        }
        else if (c == 0xF0)
        {
            // 4-byte, спец-диапазон
            if (i + 3 < n && p[i + 1] >= 0x90 && p[i + 1] <= 0xBF && is_cont_byte(p[i + 2]) && is_cont_byte(p[i + 3]))
            {
                out.append(reinterpret_cast<const char *>(p + i), 4);
                i += 4;
            }
            else
            {
                ++i;
            }
        }
        else if (c >= 0xF1 && c <= 0xF3)
        {
            // 4-byte
            if (i + 3 < n && is_cont_byte(p[i + 1]) && is_cont_byte(p[i + 2]) && is_cont_byte(p[i + 3]))
            {
                out.append(reinterpret_cast<const char *>(p + i), 4);
                i += 4;
            }
            else
            {
                ++i;
            }
        }
        else if (c == 0xF4)
        {
            // 4-byte, верхняя граница
            if (i + 3 < n && p[i + 1] >= 0x80 && p[i + 1] <= 0x8F && is_cont_byte(p[i + 2]) && is_cont_byte(p[i + 3]))
            {
                out.append(reinterpret_cast<const char *>(p + i), 4);
                i += 4;
            }
            else
            {
                ++i;
            }
        }
        else
        {
            // Неверный стартовый байт
            ++i;
        }
    }
    return out;
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
DBuse::DBuse(const std::string &host,
             const std::string &dbname,
             const std::string &user,
             const std::string &password)
    : conn_details{host, dbname, user, password}
{
    ConfigParser config;
    const int POOL_SIZE = config.get_int("DBusers.POOL_SIZE");
    // Инициализация пула соединений
    for (int i = 0; i < POOL_SIZE; ++i)
    {
        auto wrapper = std::make_shared<ConnectionWrapper>();
        initialize_connection(wrapper);
        connection_pool.push_back(wrapper);
    }
}

std::shared_ptr<DBuse::ConnectionWrapper> DBuse::get_connection()
{
    std::unique_lock<std::mutex> lock(pool_mutex);

    while (true)
    {
        for (auto &wrapper : connection_pool)
        {
            if (!wrapper->in_use.exchange(true))
            {
                return wrapper;
            }
        }
        pool_cv.wait(lock);
    }
}

void DBuse::return_connection(std::shared_ptr<ConnectionWrapper> wrapper)
{
    {
        std::lock_guard<std::mutex> lock(pool_mutex);
        wrapper->in_use = false;
    }
    pool_cv.notify_one();
}

void DBuse::initialize_connection(std::shared_ptr<ConnectionWrapper> wrapper)
{
    try
    {
        std::string connection_str =
            "host=" + conn_details.host +
            " dbname=" + conn_details.dbname +
            " user=" + conn_details.user +
            " password=" + conn_details.password +
            " client_encoding=UTF8";

        wrapper->conn = std::make_unique<pqxx::connection>(connection_str);

        if (!wrapper->conn->is_open())
        {
            throw std::runtime_error("Failed to connect to database");
        }
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Database connection error: ") + e.what());
    }
}

// DBusers.cpp
void DBuse::execute_in_transaction(const std::function<void(pqxx::work &)> &func)
{
    auto wrapper = get_connection();

    try
    {
        if (!wrapper->conn->is_open())
        {
            initialize_connection(wrapper);
        }

        pqxx::work txn(*wrapper->conn);
        try
        {
            func(txn);
            txn.commit();
        }
        catch (const pqxx::sql_error &e)
        {
            txn.abort();
            throw std::runtime_error("SQL error: " + std::string(e.what()) +
                                     " Query: " + e.query());
        }
        catch (...)
        {
            txn.abort();
            throw;
        }
    }
    catch (const pqxx::broken_connection &e)
    {
        // Повторная инициализация соединения при ошибке
        initialize_connection(wrapper);
        // Повторяем транзакцию
        pqxx::work txn(*wrapper->conn);
        func(txn);
        txn.commit();
    }
    catch (...)
    {
        throw;
    }

    // Возвращаем соединение в пул
    return_connection(wrapper);
}

json::json DBuse::get_tables()
{
    json::json result;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        pqxx::result tables = txn.exec(
            "SELECT table_name FROM information_schema.tables "
            "WHERE table_schema='public'");
        for (auto row : tables) {
            result.push_back(row[0].as<std::string>());
        } });
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
            "ORDER by ordinal_position", 
            table);
        
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
            " WHERE " + txn.quote_name(column) + " = $1",
            value);
        
        if (result.empty()) {
            throw std::runtime_error("Record not found");
        }
        
        for (const auto& field : result[0]) {
            record[field.name()] = field.is_null() ? nullptr : field.as<std::string>();
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
            username, password);
        auth_result = !res.empty(); });
    return auth_result;
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
                    table, name);
                
                if (!type_res.empty()) {
                    std::string type = type_res[0][0].as<std::string>();
                    if (type.find("char") != std::string::npos || type == "text") {
                        auto check = txn.exec_params("SELECT * FROM " + txn.quote_name(table) + 
                                   " WHERE " + txn.quote_name(name) + " = $1", val);
                        if (!check.empty()) {
                            json::json existing_record;
                            for (const auto& field : check[0]) {
                                if (field.name() != "id") {
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
            std::vector<std::string> values_list;

            for (const auto& [name, value] : values.items()) {
                if (!first) {
                    query += ", ";
                    values_part += ", ";
                }
                first = false;
                
                query += txn.quote_name(name);
                std::string val = value.is_string() ? value.get<std::string>() : value.dump();
                values_list.push_back(val);
                values_part += "$" + std::to_string(values_list.size());
            }

            query += values_part + ") RETURNING *";
            
            auto result = txn.exec_params(query, values_list);
            if (!result.empty()) {
                json::json new_record;
                for (const auto& field : result[0]) {
                    if (field.name() != "id") {
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
            "word VARCHAR(100) NOT NULL UNIQUE, "
            "status VARCHAR(20) DEFAULT 'pending', "
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
            "processed_at TIMESTAMP NULL)");
        
        txn.exec(
            "CREATE TABLE IF NOT EXISTS word_urls("
            "id SERIAL PRIMARY KEY, "
            "word_id INTEGER REFERENCES words(id) ON DELETE CASCADE, "
            "url TEXT NOT NULL, "
            "html_content TEXT, "
            "word_count INTEGER DEFAULT 0, "
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
            "UNIQUE(word_id, url))"); });
}
void DBuse::run_migrations() {
    execute_in_transaction([&](pqxx::work& txn) {
        try {
            // Проверяем существование колонки text_content
            auto res = txn.exec(
                "SELECT column_name FROM information_schema.columns "
                "WHERE table_name = 'word_urls' AND column_name = 'text_content'"
            );
            
            if (res.empty()) {
                               
                // Добавляем новую колонку для текстового контента
                txn.exec("ALTER TABLE word_urls ADD COLUMN text_content TEXT");
                
                // Обновляем существующие записи
                txn.exec(
                    "UPDATE word_urls "
                    "SET text_content = substring(html_content from 1 for 10000)"
                );
                
                
            }

            // Миграция: расширяем тип url до TEXT, если он ещё VARCHAR
            auto url_type = txn.exec(
                "SELECT data_type FROM information_schema.columns "
                "WHERE table_name = 'word_urls' AND column_name = 'url'"
            );
            if (!url_type.empty() && url_type[0][0].as<std::string>().find("char") != std::string::npos) {
                txn.exec("ALTER TABLE word_urls ALTER COLUMN url TYPE TEXT");
            }
        } catch (const std::exception& e) {
            throw;
        }
    });
}

bool DBuse::save_word_url(int word_id, 
                         const std::string& url, 
                         const std::string& html_content,
                         const std::string& text_content,
                         int word_count) {
    bool saved = false;
    execute_in_transaction([&](pqxx::work& txn) {
        std::string safe_url = sanitize_utf8(url);
        std::string safe_html = sanitize_utf8(html_content);
        std::string safe_text = sanitize_utf8(text_content);
        boost::algorithm::trim(safe_url);
        txn.exec_params(
            "INSERT INTO word_urls (word_id, url, html_content, text_content, word_count) "
            "VALUES ($1, $2, $3, $4, $5) "
            "ON CONFLICT (word_id, url) DO UPDATE "
            "SET html_content = EXCLUDED.html_content, "
            "    text_content = EXCLUDED.text_content, "
            "    word_count = EXCLUDED.word_count",
            word_id, safe_url, safe_html, safe_text, word_count);
        saved = true;
        
        // Обновляем время последнего изменения для отслеживания прогресса
        txn.exec_params(
            "UPDATE words SET processed_at = CURRENT_TIMESTAMP WHERE id = $1",
            word_id);
    });
    return saved;
}

int DBuse::get_word_id(const std::string &word)
{
    int word_id = -1;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        std::string w = sanitize_utf8(word);
        boost::algorithm::trim(w);
        auto result = txn.exec_params(
            "SELECT id FROM words WHERE word = $1", 
            w);
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
        std::string safe_url = sanitize_utf8(url);
        boost::algorithm::trim(safe_url);
        auto result = txn.exec_params(
            "SELECT 1 FROM word_urls WHERE word_id = $1 AND url = $2 LIMIT 1",
            word_id, safe_url);
        exists = !result.empty(); });
    return exists;
}

void DBuse::add_unique_constraint()
{
    // Теперь UNIQUE constraint добавляется сразу при создании таблицы
    std::cout << "UNIQUE constraint уже установлен при создании таблицы\n";
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
            last_id);
        for (auto row : result) {
            new_words.emplace_back(
                row["id"].as<int>(),
                row["word"].as<std::string>());
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

std::string DBuse::get_full_word_string(const std::string &word)
{
    std::string result;
    execute_in_transaction([&](pqxx::work &txn)
                           {
        auto res = txn.exec_params(
            "SELECT word FROM words WHERE word = $1", 
            word);
        if (!res.empty()) {
            result = res[0][0].as<std::string>();
        } });
    return result;
}

void DBuse::add_word_to_tables(const std::string &word)
{
    std::string w = sanitize_utf8(word);
    if (!filter_and_normalize_word(w)) {
        std::cerr << "Слово '" << word << "' не прошло фильтрацию\n";
        return;
    }
    
    std::cout << "[DEBUG] Добавляю слово в базу: '" << word << "' -> '" << w << "'" << std::endl;
    
    // Проверка на дубликат
    bool exists = false;
    execute_in_transaction([&](pqxx::work &txn) {
        pqxx::result res = txn.exec_params("SELECT id FROM words WHERE word = $1", w);
        exists = !res.empty();
    });
    if (exists) {
        std::cout << "[DEBUG] Слово '" << w << "' уже существует в базе" << std::endl;
        return;
    }
    
    execute_in_transaction([&](pqxx::work &txn)
                           {
        try {
            auto result = txn.exec_params(
                "INSERT INTO words (word) VALUES ($1) RETURNING id",
                w);
            std::cout << "[DEBUG] Слово '" << w << "' успешно добавлено в базу" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка при добавлении слова: " << e.what() << std::endl;
        }
    });
}

json::json DBuse::process_word_request(const std::string &word)
{
    json::json response;

    execute_in_transaction([&](pqxx::work &txn)
                           {
        pqxx::result word_result = txn.exec_params(
            "SELECT id FROM words WHERE word = $1", 
            word);

        if (!word_result.empty()) {
            int word_id = word_result[0][0].as<int>();

            pqxx::result urls_result = txn.exec_params(
                "SELECT url, LEFT(text_content, 200) AS snippet, word_count FROM word_urls "
                "WHERE word_id = $1 ORDER BY word_count DESC LIMIT 10",
                word_id);

            json::json urls_json = json::json::array();
            for (auto row : urls_result) {
                urls_json.push_back({
                    {"url", row["url"].as<std::string>()},
                    {"count", row["word_count"].as<int>()},
                    {"content", row["snippet"].as<std::string>()}
                });
            }

            if (!urls_json.empty()) {
                response = {
                    {"status", "completed"},
                    {"word_id", word_id},
                    {"urls", urls_json}
                };
            } else {
                response = {
                    {"status", "not_found"},
                    {"word_id", word_id},
                    {"urls", json::json::array()}
                };
            }
        } else {
            // Не создаем новое слово автоматически. Возвращаем что не найдено
            response = {
                {"status", "not_found"},
                {"message", "Слово отсутствует в индексе"}
            };
        } });

    return response;
}

json::json DBuse::check_word_status(int word_id) {
    json::json response;
    
    execute_in_transaction([&](pqxx::work& txn) {
        auto result = txn.exec_params(
            "SELECT status FROM words WHERE id = $1",
            word_id);
        
        if (!result.empty()) {
            std::string status = result[0][0].as<std::string>();
            response["status"] = status;
            
            // Всегда возвращаем текущие результаты, даже если обработка не завершена
            pqxx::result urls_result = txn.exec_params(
                "SELECT url, text_content, word_count FROM word_urls "
                "WHERE word_id = $1 ORDER BY word_count DESC LIMIT 10",
                word_id);
            
            json::json urls_json;
            for (auto row : urls_result) {
                urls_json.push_back({
                    {"url", row["url"].as<std::string>()},
                    {"count", row["word_count"].as<int>()},
                    {"content", row["text_content"].as<std::string>()}
                });
            }
            
            response["urls"] = urls_json;
        } else {
            throw std::runtime_error("Word not found");
        }
    });
    
    return response;
}

void DBuse::check_column_exists(pqxx::work &txn,
                                const std::string &table,
                                const std::string &column)
{
    auto res = txn.exec_params(
        "SELECT 1 FROM information_schema.columns "
        "WHERE table_name=$1 AND column_name=$2",
        table, column);

    if (res.empty())
    {
        throw std::runtime_error("Column " + column + " doesn't exist in table " + table);
    }
}