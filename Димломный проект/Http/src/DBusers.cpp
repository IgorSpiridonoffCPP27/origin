#include "DBusers.h"

DBuse::DBuse(const std::string&host, const std::string&dbname,const std::string&user,const std::string&password):
    conn("host=" + host + " " +
           "dbname=" + dbname + " " +
           "user=" + user + " " +
           "password=" + password)
{
        if (conn.is_open()) {
            std::cout << "Успешное подключение к " << conn.dbname() << std::endl;
        } else {
            std::cerr << "Не удалось подключиться к БД" << std::endl;
            
        }

}

void DBuse::create_tables(){
    pqxx::work txn(conn);
    try {
        // Создаем таблицу "слов" (одна)
        txn.exec(
            "CREATE TABLE IF NOT EXISTS WORDs("
            "id SERIAL PRIMARY KEY, "
            "name VARCHAR(100) NOT NULL)"
        );
        txn.commit();
        std::cout << "Таблица успешно создана\n";
    } catch (const std::exception& e) {
        txn.abort();
        std::cerr << "Ошибка при создании таблиц: " << e.what() << std::endl;
    }


}

void DBuse::add_unique_constraint() {
    pqxx::work txn(conn);
    try {
        txn.exec("ALTER TABLE WORDs ADD CONSTRAINT words_name_unique UNIQUE (words)");
        txn.commit();
        std::cout << "UNIQUE constraint добавлен успешно\n";
    } catch (const std::exception& e) {
        txn.abort();
        std::cerr << "Ошибка при добавлении constraint: " << e.what() << std::endl;
    }
}

void DBuse::add_word_to_tables(const std::string& word) {
    if (word.empty()) {
        std::cerr << "Пустое слово не может быть добавлено\n";
        return;
    }
    pqxx::work txn(conn);
    try {
        
        // Оптимизированный запрос с проверкой длины слова
        if (word.length() > 100) { // Предполагая, что name VARCHAR(100)
            std::cerr << "Слово '" << word << "' слишком длинное (макс. 100 символов)\n";
            return;
        }

        // Более эффективный запрос
        auto result = txn.exec_params(
            "WITH inserted AS ("
            "  INSERT INTO WORDs (words) VALUES ($1) "
            "  ON CONFLICT (words) DO NOTHING "
            "  RETURNING id"
            ") SELECT * FROM inserted UNION ALL "
            "SELECT id FROM WORDs WHERE words = $1 LIMIT 1",
            word
        );

        if (!result.empty()) {
            txn.commit();
            const int word_id = result[0]["id"].as<int>();
            std::cout << "Слово '" << word << "' обработано. ID: " << word_id << "\n";
        } else {
            txn.commit();
            std::cerr << "Неожиданная ошибка при обработке слова '" << word << "'\n";
        }
    } 
    catch (const pqxx::broken_connection& e) {
        std::cerr << "Ошибка соединения с БД: " << e.what() << "\n";
        // Здесь можно добавить переподключение
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL ошибка: " << e.what() << "\n";
        std::cerr << "Запрос: " << e.query() << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Общая ошибка: " << e.what() << "\n";
    }
}