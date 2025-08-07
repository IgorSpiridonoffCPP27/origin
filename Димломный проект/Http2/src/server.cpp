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

void handle_request(DBuse& db, http::request<http::string_body>& req, http::response<http::string_body>& res) {
    res.set(http::field::content_type, "application/json; charset=utf-8");
    
    try {
        if (req.method() == http::verb::get) {
            if (req.target() == "/tables") {
                res.body() = db.get_tables().dump();
                res.result(http::status::ok);
            } 
            else if (req.target().starts_with("/schema")) {
                std::string table = get_query_param(req.target(), "table");
                res.body() = db.get_columns(table).dump();
                res.result(http::status::ok);
            }
            else if (req.target().starts_with("/get_record")) {
                std::string table = get_query_param(req.target(), "table");
                std::string column = get_query_param(req.target(), "column");
                std::string value = get_query_param(req.target(), "value");
                res.body() = db.get_record(table, column, value).dump();
                res.result(http::status::ok);
            }
        }
        else if (req.method() == http::verb::post) {
            auto body = nlohmann::json::parse(req.body());
            
            if (req.target() == "/login") {
                if (db.check_auth(body["username"].get<std::string>(), body["password"].get<std::string>())) {
                    res.body() = nlohmann::json{{"access", "granted"}}.dump();
                    res.result(http::status::ok);
                } else {
                    res.result(http::status::unauthorized);
                    res.body() = nlohmann::json{{"error", "Неверные учетные данные"}}.dump();
                }
            }
            else if (req.target() == "/word_request") {
                auto body = nlohmann::json::parse(req.body());
                std::string word = body["word"].get<std::string>();
                res.body() = db.process_word_request(word).dump();
                res.result(http::status::ok);
            }
            else if (req.target().starts_with("/write")) {
                std::string table = get_query_param(req.target(), "table");
    nlohmann::json response;
    try {
        db.insert_data(table, body["values"], response);
        
        // Устанавливаем соответствующий HTTP статус из response["status"]
        res.result(static_cast<http::status>(response["status"].get<int>()));
        res.body() = response.dump();
        
        // Убедимся, что заголовки установлены правильно
        res.set(http::field::content_type, "application/json; charset=utf-8");
        res.prepare_payload();
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        res.body() = nlohmann::json{
            {"status", 500},
            {"message", "Internal server error"}
        }.dump();
    }
}
        }
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        res.body() = nlohmann::json{{"error", e.what()}}.dump();
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