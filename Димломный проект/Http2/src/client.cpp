#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <windows.h>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

class Client {
    net::io_context ioc;
    tcp::resolver resolver;
    
    // Функция для корректного чтения русских символов
    std::string ReadRussianInput() {
        // Временная установка кодовой страницы 1251
        UINT old_cp = GetConsoleCP();
        SetConsoleCP(1251);
        
        std::string input;
        std::getline(std::cin, input);
        
        // Восстановление исходной кодовой страницы
        SetConsoleCP(old_cp);
        
        // Преобразование из 1251 в UTF-8
        int wsize = MultiByteToWideChar(1251, 0, input.c_str(), -1, NULL, 0);
        if (wsize == 0) return "";
        
        std::wstring wstr(wsize, 0);
        MultiByteToWideChar(1251, 0, input.c_str(), -1, &wstr[0], wsize);
        
        int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        if (utf8_size == 0) return "";
        
        std::string utf8_str(utf8_size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8_str[0], utf8_size, NULL, NULL);
        
        // Удаляем нулевой символ в конце
        if (!utf8_str.empty() && utf8_str.back() == '\0') {
            utf8_str.pop_back();
        }
        
        return utf8_str;
    }

    json send_request(const std::string& target, http::verb method, const json& body = {}) {
        beast::tcp_stream stream(ioc);
        auto const results = resolver.resolve("localhost", "8080");
        stream.connect(results);

        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::host, "localhost");
        req.set(http::field::content_type, "application/json");
        
        if (!body.empty()) {
            req.body() = body.dump();
        }
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        return json::parse(res.body());
    }

public:
    Client() : resolver(ioc) {}

    void run() {
        try {
            // 1. Получаем список таблиц
            json tables = send_request("/tables", http::verb::get);
            std::cout << "Доступные таблицы:\n";
            for (size_t i = 0; i < tables.size(); ++i) {
                std::cout << "  " << i+1 << ". " << tables[i] << "\n";
            }

            // 2. Выбираем таблицу
            std::cout << "Выберите таблицу (номер): ";
            size_t table_idx;
            std::cin >> table_idx;
            if (table_idx < 1 || table_idx > tables.size()) {
                std::cerr << "Неверный номер таблицы!\n";
                return;
            }
            std::string table = tables[table_idx-1];
            std::cin.ignore();

            // 3. Получаем структуру таблицы
            json columns = send_request("/schema?table=" + table, http::verb::get);
            if (columns.empty()) {
                std::cerr << "Таблица не содержит доступных для заполнения полей\n";
                return;
            }
            
            // 4. Ввод данных
            std::cout << "Введите данные для таблицы " << table << ":\n";
            json values = json::object();

            for (const auto& col : columns) {
                std::string name = col["name"].get<std::string>();
                std::string type = col["type"].get<std::string>();
                
                while (true) {
                    std::cout << name << " (" << type << "): ";
                    std::string input = ReadRussianInput();
                    
                    if (input.empty()) {
                        std::cout << "Поле не может быть пустым!\n";
                        continue;
                    }
                    
                    // Отладочный вывод
                    std::cout << "Отправляем в базу: " << input << " (hex: ";
                    for (char c : input) {
                        printf("%02x ", (unsigned char)c);
                    }
                    std::cout << ")\n";
                    
                    values[name] = input;
                    break;
                }
            }

            // 5. Авторизация для записи
            std::cout << "\nТребуется авторизация для записи:\n";
            std::string username, password;
            std::cout << "Логин: "; 
            username = ReadRussianInput();
            std::cout << "Пароль: ";
            password = ReadRussianInput();
            
            json auth_res = send_request("/login", http::verb::post, 
                {{"username", username}, {"password", password}});
            
            if (auth_res.contains("error")) {
                std::cerr << "Ошибка авторизации: " << auth_res["error"] << "\n";
                return;
            }

            // 6. Отправка данных
            std::cout << "Отправляемые данные:\n";
            for (const auto& [key, val] : values.items()) {
                std::cout << "  " << key << ": " << val << "\n";
            }
            
            json write_res = send_request("/write?table=" + table, http::verb::post,
                {{"values", values}});

            if (write_res.contains("error")) {
                std::cerr << "\nОшибка: " << write_res["error"] << "\n";
            } else {
                std::cout << "\nРезультат: " << write_res.dump(2) << "\n";
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Ошибка: " << e.what() << "\n";
        }
    }
};

int main() {
    // Устанавливаем UTF-8 для вывода
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    
    try {
        Client client;
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Фатальная ошибка: " << e.what() << "\n";
    }
    
    std::cout << "\nНажмите Enter для выхода...";
    std::cin.ignore();
    return 0;
}