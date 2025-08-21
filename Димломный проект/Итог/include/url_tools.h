#pragma once
#include <string>
#include <iomanip>
#include <sstream>
#include <cctype>

std::string url_encode(const std::string& value);
std::string normalize_url(const std::string& url, const std::string& base_url);
std::string get_host(const std::string& url);
// Кодирует цель HTTP-запроса (path + query) в допустимый ASCII, сохраняя разделители
std::string encode_http_target(const std::string& target);