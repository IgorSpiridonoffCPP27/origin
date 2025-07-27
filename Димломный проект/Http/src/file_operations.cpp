#include "pch.h"            // Прекомпилированные заголовки (должен быть первым!)
#include "file_operations.h"
#include "utils.h"          // Для url_decode, convert_to_utf8
#include "DBusers.h"        // Для работы с БД
#include <fstream>
#include <ctime>
#include <boost/algorithm/string/replace.hpp>

// ================ Реализации функций ================

void save_to_log(const std::string& data) {
    std::ofstream log("../../log.txt", std::ios::app | std::ios::binary);
    if (!log) return;

    // Форматирование временной метки
    std::time_t now = std::time(nullptr);
    char timestamp[100];
    std::strftime(timestamp, sizeof(timestamp), 
                 "[%Y-%m-%d %H:%M:%S] ", 
                 std::localtime(&now));
    
    // Запись в UTF-8
    log << convert_to_utf8(timestamp) << data << "\n---\n";
}

// --------------------------------------------------------

std::string read_html_main() {
    const std::string path = "../../main.html";
    std::ifstream file(path, std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть main.html");
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// --------------------------------------------------------

std::string read_post_template() {
    const std::string path = "../../POST_log.html";
    std::ifstream file(path, std::ios::binary);
    
    if (!file.is_open()) {
        return "<html><body><h1>Template not found</h1></body></html>";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// --------------------------------------------------------

void handle_request(
    http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    DBuse& connectiondb
) {
    // Устанавливаем UTF-8 для всех ответов
    res.set(http::field::content_type, "text/html; charset=utf-8");
    
    try {
        // Обработка GET-запроса для главной страницы
        if (req.method() == http::verb::get && req.target() == "/") {
            res.body() = read_html_main();
        }
        
        // Обработка POST-запроса с данными формы
        else if (req.method() == http::verb::post) {
            std::string body = url_decode(req.body());
            save_to_log(convert_to_utf8(body));
            
            // Парсинг и сохранение данных
            auto keyValues = extractKeyValues(body);
            connectiondb.add_word_to_tables(keyValues["username"]);
            
            // Генерация ответа из шаблона
            std::string template_html = read_post_template();
            boost::replace_all(template_html, "{{username}}", keyValues["username"]);
            boost::replace_all(template_html, "{{age}}", keyValues["age"]);
            
            res.body() = template_html;
        }
        
        // Отдача файла логов для скачивания
        else if (req.method() == http::verb::get && req.target() == "/download") {
            std::ifstream file("../../log.txt", std::ios::binary);
            if (file) {
                res.body().assign(
                    std::istreambuf_iterator<char>(file),
                    std::istreambuf_iterator<char>()
                );
                res.set(http::field::content_type, "text/plain; charset=utf-8");
                res.set(http::field::content_disposition, "attachment; filename=log.txt");
            } else {
                res.result(http::status::not_found);
                res.body() = convert_to_utf8(u8"Файл логов не найден");
            }
        }
    } catch (const std::exception& e) {
        // Обработка ошибок
        res.result(http::status::internal_server_error);
        res.body() = "Server error: " + std::string(e.what());
    }
    
    // Финализация ответа
    res.prepare_payload();
}