#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

const std::string API_HOST = "localhost";
const std::string API_PORT = "8080";

http::response<http::string_body> call_api(const std::string& target, http::verb method, const std::string& body = "") {
    net::io_context ioc;
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    auto const results = resolver.resolve(API_HOST, API_PORT);
    stream.connect(results);

    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::host, API_HOST);
    req.set(http::field::content_type, "application/json");
    if (!body.empty()) {
        req.body() = body;
        req.prepare_payload();
    }

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    stream.socket().shutdown(tcp::socket::shutdown_both);

    return res;
}

std::string generate_html() {
    return R"html(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Database Client</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        button { padding: 8px 16px; background: #4CAF50; color: white; border: none; cursor: pointer; }
        button:hover { background: #45a049; }
        table { border-collapse: collapse; margin-top: 20px; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; }
        input { padding: 8px; width: 100%; box-sizing: border-box; }
        .error { color: red; background: #ffeeee; padding: 10px; border-radius: 4px; }
        .success { color: green; background: #eeffee; padding: 10px; border-radius: 4px; }
        .existing-record { margin-top: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px; }
    </style>
</head>
<body>
    <h1>Работа с базой данных</h1>
    
    <div>
        <button onclick="loadTables()">Показать таблицы</button>
        <div id="tables"></div>
    </div>
    
    <div id="form-container" style="margin-top: 30px;"></div>
    <div id="result"></div>

    <script>
        async function loadTables() {
            try {
                const response = await fetch('/api/tables');
                if (!response.ok) {
                    const error = await response.json();
                    throw new Error(error.error || 'Ошибка загрузки таблиц');
                }
                const tables = await response.json();
                
                let html = '<h2>Доступные таблицы:</h2><table><tr><th>Название</th><th>Действие</th></tr>';
                tables.forEach(table => {
                    html += `<tr><td>${table}</td><td><button onclick="showTableForm('${table}')">Добавить данные</button></td></tr>`;
                });
                html += '</table>';
                document.getElementById('tables').innerHTML = html;
            } catch (error) {
                document.getElementById('tables').innerHTML = `<p class="error">Ошибка: ${error.message}</p>`;
            }
        }

        async function showTableForm(table) {
            try {
                const response = await fetch('/api/schema?table=' + table);
                if (!response.ok) {
                    const error = await response.json();
                    throw new Error(error.error || 'Ошибка загрузки схемы таблицы');
                }
                const columns = await response.json();
                
                let form = `<h2>Добавление в ${table}</h2><form onsubmit="submitForm(event, '${table}')">`;
                columns.forEach(col => {
                    if (col.name !== "id") { // Исключаем поле id
                        form += `<div class="form-group"><label>${col.name} (${col.type}):</label><input type="text" name="${col.name}" required></div>`;
                    }
                });
                form += `
                    <div class="form-group"><label>Логин:</label><input type="text" name="username" required></div>
                    <div class="form-group"><label>Пароль:</label><input type="password" name="password" required></div>
                    <button type="submit">Отправить</button>
                    </form>
                `;
                document.getElementById('form-container').innerHTML = form;
            } catch (error) {
                document.getElementById('form-container').innerHTML = `<p class="error">Ошибка: ${error.message}</p>`;
            }
        }

        async function submitForm(e, table) {
            e.preventDefault();
            const formData = new FormData(e.target);
            const data = {};
            const auth = {};
            
            formData.forEach((value, key) => {
                if (key === 'username' || key === 'password') {
                    auth[key] = value;
                } else {
                    data[key] = value;
                }
            });
            
            try {
                // Авторизация
                const authResponse = await fetch('/api/login', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(auth)
                });
                
                if (!authResponse.ok) {
                    const error = await authResponse.json();
                    throw new Error(error.error || 'Ошибка авторизации');
                }
                
                // Отправка данных
                const writeResponse = await fetch('/api/write?table=' + table, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ values: data })
                });
                
                const result = await writeResponse.json();
                
                if (!writeResponse.ok) {
                    if (result.existing_record) {
                        let html = `<div class="error">${result.message || result.error}</div><div class="existing-record"><h3>Существующая запись:</h3><table>`;
                        for (const [key, value] of Object.entries(result.existing_record)) {
                            if (key !== "id") { // Исключаем поле id
                                html += `<tr><td><strong>${key}</strong></td><td>${value !== null ? value : 'NULL'}</td></tr>`;
                            }
                        }
                        html += '</table></div>';
                        document.getElementById('result').innerHTML = html;
                    } else {
                        throw new Error(result.message || result.error || 'Ошибка записи');
                    }
                } else {
                    document.getElementById('result').innerHTML = `<div class="success">${result.message}</div>`;
                    e.target.reset();
                }
            } catch (error) {
                document.getElementById('result').innerHTML = `<div class="error">${error.message}</div>`;
            }
        }
    </script>
</body>
</html>
)html";
}

void handle_request(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    try {
        if (req.method() == http::verb::get && req.target() == "/") {
            res.set(http::field::content_type, "text/html; charset=utf-8");
            res.body() = generate_html();
        }
        else if (req.target().starts_with("/api")) {
            // Проксируем запрос к API серверу
            std::string target = req.target().substr(4); // Убираем "/api"
            auto api_res = call_api(target, req.method(), req.body());
            
            // Копируем весь ответ от API сервера
            res.result(api_res.result());
            for (const auto& field : api_res.base()) {
                res.set(field.name(), field.value());
            }
            res.body() = api_res.body();
        }
        else {
            res.result(http::status::not_found);
            res.body() = json{{"error", "Not Found"}}.dump();
        }
        res.prepare_payload();
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        res.body() = json{{"error", e.what()}}.dump();
        res.prepare_payload();
    }
}

int main() {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, {tcp::v4(), 8081});
        std::cout << "Web server running at http://localhost:8081\n";

        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);

            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(socket, buffer, req);

            http::response<http::string_body> res;
            handle_request(req, res);

            http::write(socket, res);
            socket.shutdown(tcp::socket::shutdown_send);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}