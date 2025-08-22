#include "word_tools.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <iostream> // Added for debug output
#include <unicode/unistr.h>
#include <cctype>



// ICU-реализация: корректно понижает регистр для любых Unicode-символов
static std::string to_lower_utf8_icu(const std::string& s) {
	icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(icu::StringPiece(s.c_str(), static_cast<int32_t>(s.size())));
	ustr.toLower();
	std::string out;
	ustr.toUTF8String(out);
	return out;
}

// Проверяет, является ли символ буквой (латиница или кириллица)
static bool is_letter_utf8(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    
    // ASCII латиница a-z, A-Z
    if ((uc >= 'a' && uc <= 'z') || (uc >= 'A' && uc <= 'Z')) {
        return true;
    }
    
    // Цифры
    if (uc >= '0' && uc <= '9') {
        return false; // Цифры не считаются буквами
    }
    
    // Дефис
    if (c == '-') {
        return false; // Дефис не считается буквой
    }
    
    // Для UTF-8 кириллицы: если это не ASCII и не специальный символ, считаем буквой
    // Это упрощенный подход, но работает для большинства случаев
    return uc > 127;
}

// Проверяет, является ли символ допустимым для слова
static bool is_valid_word_char(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    
    // ASCII латиница a-z, A-Z
    if ((uc >= 'a' && uc <= 'z') || (uc >= 'A' && uc <= 'Z')) {
        return true;
    }
    
    // Цифры
    if (uc >= '0' && uc <= '9') {
        return true;
    }
    
    // Дефис
    if (c == '-') {
        return true;
    }
    
    // Для UTF-8 кириллицы: если это не ASCII, считаем допустимым
    return uc > 127;
}

// Проверяет, является ли строка числом (только цифры)
static bool is_number(const std::string& str) {
    if (str.empty()) return false;
    
    for (char c : str) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

bool filter_and_normalize_word(std::string& word) {
    std::string original = word;
    boost::algorithm::trim(word);
    
    // Убираем кавычки и похожие обрамители, включая русские «» и типографские ковычки
    static const char* kQuoteMarks[] = {
        "\"", "'", "`",
        "«", "»",
        "„", "“", "”", "‘", "’"
    };
    for (const char* q : kQuoteMarks) {
        boost::algorithm::replace_all(word, q, "");
    }
    
    // Проверяем длину
    if (word.length() < 4 || word.length() > 31) {
        std::cout << "[DEBUG] Слово '" << original << "' -> '" << word << "' отфильтровано по длине (" << word.length() << ")" << std::endl;
        return false;
    }
    
    // Проверяем, является ли строка числом
    if (is_number(word)) {
        std::cout << "[DEBUG] Число '" << original << "' -> '" << word << "' прошло фильтрацию" << std::endl;
        return true;
    }
    
    // Приводим к нижнему регистру все буквы (ICU)
    word = to_lower_utf8_icu(word);
    
    // Проверяем, что слово содержит хотя бы одну букву (латиница или кириллица)
    bool has_letter = false;
    for (char c : word) {
        if (is_letter_utf8(c)) {
            has_letter = true;
            break;
        }
    }
    
    if (!has_letter) {
        std::cout << "[DEBUG] Слово '" << original << "' -> '" << word << "' отфильтровано (нет букв)" << std::endl;
        return false;
    }
    
    // Разрешаем буквы, цифры и дефис
    for (char c : word) {
        if (!is_valid_word_char(c)) {
            std::cout << "[DEBUG] Слово '" << original << "' -> '" << word << "' отфильтровано (недопустимый символ: " << c << ")" << std::endl;
            return false;
        }
    }
    
    std::cout << "[DEBUG] Слово '" << original << "' -> '" << word << "' прошло фильтрацию" << std::endl;
    return !word.empty();
}
