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
    return R"delimiter(<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Word Search</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        #search-form { margin: 20px 0; }
        #search-input { padding: 8px; width: 300px; }
        #search-button { padding: 8px 16px; }
        #result { margin-top: 20px; }
        .url-item { margin: 10px 0; border-bottom: 1px solid #eee; padding-bottom: 10px; }
        .url { font-weight: bold; color: #06c; }
        .count { color: #666; }
        .pending { color: #ff9900; }
        .processing { color: #0099ff; }
    </style>
</head>
<body>
    <h1>Поиск слова в базе</h1>
    
    <div id="search-form">
        <input type="text" id="search-input" placeholder="Введите слово...">
        <button id="search-button">Найти</button>
    </div>
    
    <div id="result"></div>

    <script>
        async function checkStatus(word_id) {
            try {
                const response = await fetch('/api/check_status', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ word_id })
                });
                
                return await response.json();
            } catch (error) {
                console.error('Error checking status:', error);
                return { status: 'error' };
            }
        }

        async function pollStatus(word_id) {
            const resultDiv = document.getElementById('result');
            
            while (true) {
                const data = await checkStatus(word_id);
                
                if (data.status === 'completed') {
                    // Показываем результаты
                    let html = `<h3>Результаты обработки:</h3>`;
                    if (data.urls && data.urls.length > 0) {
                        data.urls.forEach(item => {
                            html += `
                                <div class="url-item">
                                    <div class="url">${item.url}</div>
                                    <div class="count">Найдено ${item.count} раз</div>
                                    <div class="content">${item.content.substring(0, 100)}...</div>
                                </div>
                            `;
                        });
                    } else {
                        html += `<p>По этому слову не найдено данных</p>`;
                    }
                    resultDiv.innerHTML = html;
                    break;
                } else if (data.status === 'processing') {
                    resultDiv.innerHTML = `<p class="processing">Слово обрабатывается, пожалуйста, подождите...</p>`;
                } else if (data.status === 'pending') {
                    resultDiv.innerHTML = `<p class="pending">Слово в очереди на обработку...</p>`;
                } else {
                    resultDiv.innerHTML = `<p style="color: red;">Ошибка при проверке статуса</p>`;
                    break;
                }
                
                // Проверяем статус каждые 2 секунды
                await new Promise(resolve => setTimeout(resolve, 2000));
            }
        }

        document.getElementById('search-button').addEventListener('click', async () => {
            const word = document.getElementById('search-input').value.trim();
            if (!word) return;
            
            const resultDiv = document.getElementById('result');
            resultDiv.innerHTML = '<p>Ищем...</p>';
            
            try {
                const response = await fetch('/api/word_request', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ word })
                });
                
                const data = await response.json();
                
                if (data.status === 'completed') {
                    // Показываем результаты сразу
                    let html = `<h3>Результаты для "${word}":</h3>`;
                    if (data.urls && data.urls.length > 0) {
                        data.urls.forEach(item => {
                            html += `
                                <div class="url-item">
                                    <div class="url">${item.url}</div>
                                    <div class="count">Найдено ${item.count} раз</div>
                                    <div class="content">${item.content.substring(0, 100)}...</div>
                                </div>
                            `;
                        });
                    } else {
                        html += `<p>Нет данных по этому слову</p>`;
                    }
                    resultDiv.innerHTML = html;
                } else {
                    // Начинаем опрос статуса
                    resultDiv.innerHTML = `<p class="pending">Слово "${word}" добавлено в очередь на обработку. Ожидайте...</p>`;
                    pollStatus(data.word_id);
                }
            } catch (error) {
                resultDiv.innerHTML = `<p style="color: red;">Ошибка: ${error.message}</p>`;
            }
        });
    </script>
</body>
</html>)delimiter";
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