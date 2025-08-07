#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <cstdlib>
#include "DBusers.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

std::string get_query_param(const std::string& target, const std::string& param) {
    size_t param_pos = target.find(param + "=");
    if (param_pos == std::string::npos) return "";
    param_pos += param.length() + 1;
    size_t end_pos = target.find("&", param_pos);
    if (end_pos == std::string::npos) end_pos = target.length();
    return target.substr(param_pos, end_pos - param_pos);
}

// В handle_request добавим обработку новых запросов
void handle_request(DBuse& db, http::request<http::string_body>& req, http::response<http::string_body>& res) {
    res.set(http::field::content_type, "application/json; charset=utf-8");
    
    try {
        if (req.method() == http::verb::post) {
            if (req.target() == "/word_request") {
                auto body = nlohmann::json::parse(req.body());
                std::string word = body["word"].get<std::string>();
                
                // Получаем данные из базы
                auto result = db.process_word_request(word);
                
                // Форматируем ответ для нового интерфейса
                if (result["status"] == "completed" && result.contains("urls")) {
                    nlohmann::json formatted;
                    formatted["status"] = "completed";
                    formatted["urls"] = nlohmann::json::array();
                    
                    for (auto& item : result["urls"]) {
                        formatted["urls"].push_back({
                            {"url", item["url"]},
                            {"count", item["count"]},
                            {"content", item["content"]}
                        });
                    }
                    
                    res.body() = formatted.dump();
                } else {
                    res.body() = result.dump();
                }
                
                res.result(http::status::ok);
            }
            else if (req.target() == "/check_status") {
                auto body = nlohmann::json::parse(req.body());
                int word_id = body["word_id"].get<int>();
                
                auto result = db.check_word_status(word_id);
                
                // Форматируем ответ для нового интерфейса
                if (result["status"] == "completed" && result.contains("urls")) {
                    nlohmann::json formatted;
                    formatted["status"] = "completed";
                    formatted["urls"] = nlohmann::json::array();
                    
                    for (auto& item : result["urls"]) {
                        formatted["urls"].push_back({
                            {"url", item["url"]},
                            {"count", item["count"]},
                            {"content", item["content"]}
                        });
                    }
                    
                    res.body() = formatted.dump();
                } else {
                    res.body() = result.dump();
                }
                
                res.result(http::status::ok);
            }
        }
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        res.body() = nlohmann::json{
            {"status", "error"},
            {"message", e.what()}
        }.dump();
    }
}
int main() {
    try {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        std::cout << "Запуск сервера..." << std::endl;
        
        try {
            // Инициализация базы данных с подробным выводом
            std::cout << "Попытка подключения к базе данных..." << std::endl;
            DBuse db("localhost", "HTTP", "test_postgres", "12345678");
            
            std::cout << "Подключение к базе данных успешно." << std::endl;
            
            // Создание таблиц (если нужно)
            std::cout << "Проверка структуры базы данных..." << std::endl;
            db.create_tables();
            db.add_unique_constraint();
            std::cout << "Структура базы данных проверена." << std::endl;

            net::io_context ioc;
            tcp::acceptor acceptor(ioc, {tcp::v4(), 8080});
            std::cout << "Сервер запущен на порту 8080" << std::endl;
            std::cout << "Ожидание подключений..." << std::endl;

            while (true) {
                try {
                    tcp::socket socket(ioc);
                    acceptor.accept(socket);

                    beast::flat_buffer buffer;
                    http::request<http::string_body> req;
                    http::read(socket, buffer, req);

                    http::response<http::string_body> res;
                    handle_request(db, req, res);
                    res.prepare_payload();

                    http::write(socket, res);
                    socket.shutdown(tcp::socket::shutdown_send);
                } catch (const std::exception& e) {
                    std::cerr << "Ошибка при обработке запроса: " << e.what() << std::endl;
                }
            }
        } catch (const pqxx::broken_connection& e) {
            std::cerr << "\nОШИБКА: Не удалось подключиться к PostgreSQL серверу." << std::endl;
            std::cerr << "Проверьте:\n"
                      << "1. Запущен ли сервер PostgreSQL\n"
                      << "2. Параметры подключения (хост, имя базы, пользователь, пароль)\n"
                      << "3. Существует ли база данных 'HTTP'\n"
                      << "4. Имеет ли пользователь 'test_postgres' права на подключение\n"
                      << "Текст ошибки: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "\nКРИТИЧЕСКАЯ ОШИБКА: " << e.what() << std::endl;
        }
        
        // Ожидаем ввода перед закрытием, чтобы увидеть сообщения об ошибках
        std::cout << "\nНажмите Enter для выхода...";
        std::cin.ignore();
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Фатальная ошибка: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}