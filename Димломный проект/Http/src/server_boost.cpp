#include "pch.h"
#include "DBusers.h"
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

std::unordered_map<std::string, std::string> extractKeyValues(const std::string& input) {
    std::unordered_map<std::string, std::string> keyValues;
    size_t start = 0;
    size_t end = input.find('&');

    while (true) {
        std::string segment;
        if (end == std::string::npos) {
            segment = input.substr(start);
        } else {
            segment = input.substr(start, end - start);
        }

        size_t equal_pos = segment.find('=');
        if (equal_pos != std::string::npos) {
            std::string key = segment.substr(0, equal_pos);
            std::string value = segment.substr(equal_pos + 1);
            keyValues[key] = value;
        }

        if (end == std::string::npos) break;
        start = end + 1;
        end = input.find('&', start);
    }

    return keyValues;
}


std::atomic<bool> server_running(true);

void server_control_thread() {
    std::cout << "Press 'q' and Enter to stop the server..." << std::endl;
    char input;
    while (server_running) {
        std::cin.get(input);
        if (input == 'q') {
            server_running = false;
            break;
        }
    }
}

void set_utf8_locale() {
    std::locale::global(std::locale("en_US.UTF-8"));
    std::wcout.imbue(std::locale());
    std::cout.imbue(std::locale());
}

std::string convert_to_utf8(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(converter.from_bytes(str));
}


void save_to_log(const std::string& data) {
    std::ofstream log("../../log.txt", std::ios::app | std::ios::binary);
    if (log) {
        std::time_t now = std::time(nullptr);
        char timestamp[100];
        std::strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S] ", std::localtime(&now));
        
        // Записываем в UTF-8
        log << convert_to_utf8(timestamp) << data << "\n---\n";
    }
}


std::string url_decode(const std::string &str) {
    std::ostringstream decoded;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int hex_val;
            std::istringstream hex_stream(str.substr(i + 1, 2));
            if (hex_stream >> std::hex >> hex_val) {
                decoded << static_cast<char>(hex_val);
                i += 2;
            } else {
                decoded << str[i];
            }
        } else if (str[i] == '+') {
            decoded << ' ';
        } else {
            decoded << str[i];
        }
    }
    return decoded.str();
}



std::string read_html_main() {
    std::string path = "../../main.html";
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть template.html");
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void handle_request(http::request<http::string_body>& req, http::response<http::string_body>& res,DBuse&connectiondb) {
    // Устанавливаем UTF-8 для всех ответов
    res.set(http::field::content_type, "text/html; charset=utf-8");
    
    if (req.method() == http::verb::get && req.target() == "/") {
        // Главная страница с формой
        res.body() = read_html_main();
    }
    else if (req.method() == http::verb::post) {
        // Обработка POST-запроса с явным указанием кодировки
        std::string content_type = req[http::field::content_type];
        std::string body = req.body();
        
        if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
            body = url_decode(body);
        }
        
        // Сохраняем в лог (файл открывается в бинарном режиме)
        save_to_log(convert_to_utf8(body));

        // Чтение HTML-шаблона (исправленная версия)
        std::ifstream template_file("../../POST_log.html");
        std::string template_html;
        
        if (template_file) {
            template_html.assign(
                std::istreambuf_iterator<char>(template_file),
                std::istreambuf_iterator<char>()
            );
            auto keyValues = extractKeyValues(body);
            connectiondb.add_word_to_tables(keyValues["username"]);
            boost::replace_all(template_html, "{{username}}", keyValues["username"]);
            boost::replace_all(template_html, "{{age}}", keyValues["age"]);
            

        } else {
            template_html = "<html><body><h1>Ошибка: файл шаблона не найден</h1></body></html>";
        }
        
        res.body() = template_html;

    }
    else if (req.method() == http::verb::get && req.target() == "/download") {
        // Отправка файла логов с явным указанием кодировки
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
    
    res.prepare_payload();
}

int main() {
    set_utf8_locale();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    DBuse connectiondb("localhost", "HTTP", "test_postgres", "12345678");
    connectiondb.create_tables();
    connectiondb.add_unique_constraint();

    std::cout << "Using Boost version: " << BOOST_VERSION << std::endl;
    std::cout << "Server started at http://localhost:8080\n";

    try {
        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {tcp::v4(), 8080}};
        
        // Запускаем поток для управления сервером
        std::thread control_thread(server_control_thread);

        while (server_running) {
            // Проверяем каждые 100ms, не нужно ли остановить сервер
            boost::asio::deadline_timer timer(ioc);
            timer.expires_from_now(boost::posix_time::milliseconds(100));
            timer.wait();

            if (!server_running) break;

            // Принимаем новое соединение (неблокирующий режим)
            tcp::socket socket(ioc);
            boost::system::error_code ec;
            acceptor.async_accept(socket, [&](const boost::system::error_code& accept_ec) {
                ec = accept_ec;
            });
            ioc.run_one();

            if (ec) continue; // Пропускаем ошибки accept

            // Обрабатываем запрос
            try {
                beast::flat_buffer buffer;
                http::request<http::string_body> req;
                http::read(socket, buffer, req);

                http::response<http::string_body> res;
                handle_request(req, res, connectiondb);
                res.set(http::field::connection, "close");
                http::write(socket, res);

                socket.shutdown(tcp::socket::shutdown_send);
            } catch (const std::exception& e) {
                std::cerr << "Connection error: " << e.what() << std::endl;
            }
        }

        // Грациозная остановка
        acceptor.close();
        ioc.stop();
        control_thread.join();
        std::cout << "Server stopped gracefully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}