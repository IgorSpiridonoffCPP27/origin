#include "pch.h"

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
    <title>Поиск слов в базе</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            line-height: 1.6;
            color: #333;
        }
        
        h1 {
            color: #2c3e50;
            text-align: center;
            margin-bottom: 30px;
        }
        
        .search-container {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
        }
        
        #search-input {
            flex-grow: 1;
            padding: 10px 15px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 16px;
        }
        
        #search-button {
            padding: 10px 20px;
            background-color: #3498db;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            transition: background-color 0.3s;
        }
        
        #search-button:hover {
            background-color: #2980b9;
        }
        
        .status-message {
            padding: 15px;
            border-radius: 4px;
            margin-bottom: 20px;
            text-align: center;
        }
        
        .pending {
            background-color: #fff3cd;
            color: #856404;
        }
        
        .processing {
            background-color: #cce5ff;
            color: #004085;
        }
        
        .error {
            background-color: #f8d7da;
            color: #721c24;
        }
        
        .results-container {
            border: 1px solid #eee;
            border-radius: 4px;
            padding: 20px;
        }
        
        .result-item {
            margin-bottom: 20px;
            padding-bottom: 20px;
            border-bottom: 1px solid #eee;
        }
        
        .result-item:last-child {
            border-bottom: none;
            margin-bottom: 0;
            padding-bottom: 0;
        }
        
        .result-url {
            display: block;
            color: #1a73e8;
            text-decoration: none;
            font-weight: 500;
            margin-bottom: 5px;
            word-break: break-all;
        }
        
        .result-url:hover {
            text-decoration: underline;
        }
        
        .result-meta {
            font-size: 14px;
            color: #666;
            margin-bottom: 8px;
        }
        
        .result-snippet {
            font-size: 15px;
            color: #444;
            line-height: 1.5;
        }
        
        .spinner {
            display: inline-block;
            margin-right: 10px;
            animation: spin 1s linear infinite;
        }
        
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <h1>Поиск слов в базе данных</h1>
    
    <div class="search-container">
        <input type="text" id="search-input" placeholder="Введите слово для поиска..." autofocus>
        <button id="search-button">Поиск</button>
    </div>
    
    <div id="status-message"></div>
    <div id="results"></div>

    <script>
        // Элементы DOM
        const searchInput = document.getElementById('search-input');
        const searchButton = document.getElementById('search-button');
        const statusMessage = document.getElementById('status-message');
        const resultsContainer = document.getElementById('results');
        
        // Обработчик нажатия кнопки поиска
        searchButton.addEventListener('click', handleSearch);
        
        // Обработчик нажатия Enter в поле ввода
        searchInput.addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                handleSearch();
            }
        });
        
        // Функция обработки поиска
        async function handleSearch() {
            const query = searchInput.value.trim();
            if (!query) return;
            
            // Очищаем предыдущие результаты
            resultsContainer.innerHTML = '';
            
            // Показываем статус загрузки
            showStatus('Ищем...', 'pending');
            
            try {
                // Отправляем запрос на сервер
                const response = await fetch('/api/word_request', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ word: query })
                });
                
                const data = await response.json();
                
                if (data.status === 'completed') {
                    // Показываем результаты
                    displayResults(query, data.urls || []);
                } else {
                    // Начинаем опрос статуса
                    pollStatus(data.word_id, query);
                }
            } catch (error) {
                showStatus(`Ошибка: ${error.message}`, 'error');
            }
        }
        
        // Функция для отображения статуса
        function showStatus(message, type = 'pending') {
            statusMessage.innerHTML = `
                <div class="status-message ${type}">
                    ${type === 'processing' ? '<div class="spinner">↻</div>' : ''}
                    ${message}
                </div>
            `;
        }
        
        // Функция для опроса статуса
        async function pollStatus(wordId, query) {
            showStatus(`Слово "${query}" в обработке...`, 'processing');
            
            const checkStatus = async () => {
                try {
                    const response = await fetch('/api/check_status', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ word_id: wordId })
                    });
                    
                    const data = await response.json();
                    
                    if (data.status === 'completed') {
                        // Показываем результаты
                        displayResults(query, data.urls || []);
                    } else if (data.status === 'error') {
                        showStatus('Произошла ошибка при обработке запроса', 'error');
                    } else {
                        // Продолжаем опрос через 2 секунды
                        setTimeout(checkStatus, 2000);
                    }
                } catch (error) {
                    showStatus(`Ошибка при проверке статуса: ${error.message}`, 'error');
                }
            };
            
            // Начинаем опрос
            checkStatus();
        }
        
        // Функция для отображения результатов
        function displayResults(query, results) {
            if (results.length === 0) {
                showStatus(`По запросу "${query}" ничего не найдено`, 'pending');
                resultsContainer.innerHTML = '';
                return;
            }
            
            // Очищаем статус
            statusMessage.innerHTML = '';
            
            // Формируем HTML с результатами
            let html = `
                <div class="results-container">
                    <h2>Результаты для "${escapeHtml(query)}":</h2>
            `;
            
            results.forEach(item => {
                const url = escapeHtml(item.url || '');
                const count = item.count || 0;
                const snippet = truncate(escapeHtml(item.content || ''), 200);
                
                html += `
                    <div class="result-item">
                        <a href="${url}" class="result-url" target="_blank" rel="noopener noreferrer">
                            ${url}
                        </a>
                        <div class="result-meta">Найдено ${count} раз</div>
                        <div class="result-snippet">${snippet}</div>
                    </div>
                `;
            });
            
            html += `</div>`;
            resultsContainer.innerHTML = html;
        }
        
        // Вспомогательные функции
        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }
        
        function truncate(text, length) {
            return text.length > length ? text.substring(0, length) + '...' : text;
        }
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