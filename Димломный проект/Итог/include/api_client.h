#pragma once
#include <boost/beast/http.hpp>
#include <string>

namespace http = boost::beast::http;

http::response<http::string_body> call_api(
    const std::string& host,
    const std::string& port,
    const std::string& target,
    http::verb method,
    const std::string& body = "");