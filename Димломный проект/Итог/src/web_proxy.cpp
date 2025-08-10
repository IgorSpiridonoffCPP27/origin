#include "pch.h"
#include <boost/asio/ssl.hpp>
#include <atomic>
#include <thread>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

// Конфигурация прокси-сервера
struct ProxyConfig : public ConfigParser {
    unsigned short http_port;
    unsigned short https_port;
    std::string cert_file;
    std::string key_file;
    std::string api_host;
    std::string api_port;
    int thread_pool_size;

    ProxyConfig(const std::string& filename = "../../config.ini") 
        : ConfigParser(filename) 
    {
        http_port = get_int("web_proxy.http_port");
        https_port = get_int("web_proxy.https_port");
        cert_file = get("web_proxy.cert_file");
        key_file = get("web_proxy.key_file");
        api_host = get("web_proxy.api_host");
        api_port = get("web_proxy.api_port");
        thread_pool_size = get_int("web_proxy.thread_pool_size");
    }
};

// Глобальный флаг для контроля работы прокси
std::atomic<bool> proxy_running{true};

// Обработчик сигналов для корректного завершения
void handle_signals(const boost::system::error_code& error, int signal) {
    if (!error && (signal == SIGINT || signal == SIGTERM)) {
        proxy_running = false;
    }
}

// Функция для генерации HTML-страницы
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

// Функция для проксирования запросов к API
http::response<http::string_body> call_api(
    const std::string& host,
    const std::string& port,
    const std::string& target,
    http::verb method,
    const std::string& body = "")
{
    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve(host, port);
        stream.connect(results);

        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::user_agent, "WebProxy/1.0");
        
        if (!body.empty()) {
            req.body() = body;
            req.prepare_payload();
        }

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        // Исправление: правильное завершение соединения
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_send, ec);

        return res;
    } catch(const std::exception& e) {
        http::response<http::string_body> res;
        res.result(http::status::bad_gateway);
        res.body() = nlohmann::json{
            {"error", "Proxy error"},
            {"message", e.what()}
        }.dump();
        res.prepare_payload();
        return res;
    }
}

// Обработчик запросов
template<typename Stream>
void handle_request(
    Stream& stream,
    const std::string& api_host,
    const std::string& api_port,
    beast::flat_buffer& buffer)
{
    try {
        http::request<http::string_body> req;
        http::read(stream, buffer, req);

        http::response<http::string_body> res;

        if (req.method() == http::verb::get && req.target() == "/") {
            res.set(http::field::content_type, "text/html; charset=utf-8");
            res.body() = generate_html();
            res.prepare_payload();
        }
        else if (req.target().starts_with("/api")) {
            std::string target = req.target().substr(4);
            auto api_res = call_api(api_host, api_port, target, req.method(), req.body());
            
            res.result(api_res.result());
            for (const auto& field : api_res.base()) {
                res.set(field.name(), field.value());
            }
            res.body() = api_res.body();
            res.prepare_payload();
        }
        else {
            res.result(http::status::not_found);
            res.set(http::field::content_type, "application/json");
            res.body() = nlohmann::json{
                {"error", "Not Found"},
                {"path", std::string(req.target())}
            }.dump();
            res.prepare_payload();
        }

        http::write(stream, res);
    } catch(const beast::system_error& e) {
        if(e.code() != http::error::end_of_stream) {
            std::cerr << "Request handling error: " << e.what() << std::endl;
        }
    }
}

// Шаблонная функция для обработки сессии
template<typename Stream>
void do_session(
    Stream& stream,
    const std::string& api_host,
    const std::string& api_port,
    beast::flat_buffer& buffer)
{
    try {
        handle_request(stream, api_host, api_port, buffer);
        beast::error_code ec;
        // Исправление: правильное завершение соединения
        if constexpr (std::is_same_v<Stream, ssl::stream<tcp::socket>>) {
            // Для SSL: сначала shutdown SSL, затем TCP
            stream.shutdown(ec);
            if (ec == net::error::eof) {
                ec = {};
            }
            stream.next_layer().shutdown(tcp::socket::shutdown_send, ec);
        } else {
            // Для обычного TCP
            stream.shutdown(tcp::socket::shutdown_send, ec);
        }
    } catch(const std::exception& e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
}

// HTTP обработчик соединений
void run_http_proxy(
    net::io_context& ioc,
    const ProxyConfig& config)
{
    tcp::acceptor acceptor(ioc, {tcp::v4(), config.http_port});
    std::cout << "HTTP proxy listening on port " << config.http_port << std::endl;
    
    beast::flat_buffer buffer;
    
    while (proxy_running) {
        beast::error_code ec;
        tcp::socket socket(ioc);
        acceptor.accept(socket, ec);
        
        if (ec) {
            if (proxy_running) {
                std::cerr << "HTTP accept error: " << ec.message() << std::endl;
            }
            continue;
        }
        
        do_session(socket, config.api_host, config.api_port, buffer);
    }
}

// HTTPS обработчик соединений
void run_https_proxy(
    net::io_context& ioc,
    const ProxyConfig& config)
{
    try {
        ssl::context ctx{ssl::context::tlsv12_server};
        ctx.set_options(
            ssl::context::default_workarounds |
            ssl::context::no_sslv2 |
            ssl::context::single_dh_use);
        
        ctx.use_certificate_chain_file(config.cert_file);
        ctx.use_private_key_file(config.key_file, ssl::context::pem);
        
        tcp::acceptor acceptor(ioc, {tcp::v4(), config.https_port});
        std::cout << "HTTPS proxy listening on port " << config.https_port << std::endl;
        
        beast::flat_buffer buffer;
        
        while (proxy_running) {
            beast::error_code ec;
            tcp::socket socket(ioc);
            acceptor.accept(socket, ec);
            
            if (ec) {
                if (proxy_running) {
                    std::cerr << "HTTPS accept error: " << ec.message() << std::endl;
                }
                continue;
            }
            
            auto ssl_stream = std::make_unique<ssl::stream<tcp::socket>>(std::move(socket), ctx);
            ssl_stream->handshake(ssl::stream_base::server, ec);
            
            if (ec) {
                std::cerr << "SSL handshake error: " << ec.message() << std::endl;
                continue;
            }
            
            do_session(*ssl_stream, config.api_host, config.api_port, buffer);
        }
    } catch (const std::exception& e) {
        std::cerr << "HTTPS proxy error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        // Настройка консоли для поддержки UTF-8
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        // Конфигурация прокси
        ProxyConfig config;
        
        // Обработка сигналов для graceful shutdown
        net::io_context signal_ioc;
        net::signal_set signals(signal_ioc, SIGINT, SIGTERM);
        signals.async_wait(handle_signals);
        std::thread signal_thread([&signal_ioc]() { signal_ioc.run(); });
        
        // Запуск HTTP прокси
        std::thread http_thread([&]() {
            net::io_context ioc;
            run_http_proxy(ioc, config);
        });
        
        // Запуск HTTPS прокси
        std::thread https_thread([&]() {
            net::io_context ioc;
            run_https_proxy(ioc, config);
        });
        
        std::cout << "Web proxy started. Press Ctrl+C to exit." << std::endl;
        
        // Ожидание завершения работы
        http_thread.join();
        https_thread.join();
        
        // Остановка обработки сигналов
        signal_ioc.stop();
        signal_thread.join();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal proxy error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}