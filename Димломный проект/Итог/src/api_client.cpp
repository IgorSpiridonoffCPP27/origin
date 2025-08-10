#include "api_client.h"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

http::response<http::string_body> call_api(
    const std::string& host,
    const std::string& port,
    const std::string& target,
    http::verb method,
    const std::string& body)
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