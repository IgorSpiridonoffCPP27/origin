#include "pch.h"      // Все основные зависимости уже здесь
#include "utils.h"    // Собственный заголовок
#include <sstream>    // Для std::istringstream
#include <iomanip>    // Для std::hex
#include <stdexcept>  // Для std::runtime_error

// ================= Реализации функций =================

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

// --------------------------------------------------------

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

// --------------------------------------------------------

std::string convert_to_utf8(const std::string& str) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(converter.from_bytes(str));
    } catch (...) {
        return str; // Возвращаем как есть при ошибке конвертации
    }
}

// --------------------------------------------------------

void set_utf8_locale() {
    try {
        // Для Linux/macOS
        std::locale::global(std::locale("en_US.UTF-8"));
        std::wcout.imbue(std::locale());
        std::cout.imbue(std::locale());

        // Для Windows
        #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        #endif
    } catch (...) {
        std::cerr << "Failed to set UTF-8 locale. Using default.\n";
    }
}