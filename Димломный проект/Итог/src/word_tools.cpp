#include "word_tools.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <iostream> // Added for debug output
#include <unicode/unistr.h>

static std::string to_lower_utf8(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        
        // ASCII латиница A-Z -> a-z
        if (c >= 'A' && c <= 'Z') {
            r.push_back(static_cast<char>(c + 32));
        }
        // Кириллица А-Я -> а-я (UTF-8 байты 0xD0 и 0xD1)
        else if (c == 0xD0) {
            // Первый байт UTF-8 для кириллицы А-Я
            if (i + 1 < s.size()) {
                unsigned char next_c = static_cast<unsigned char>(s[i + 1]);
                if (next_c >= 0x90 && next_c <= 0xBF) { // А-Я
                    r.push_back(static_cast<char>(0xD0));
                    r.push_back(static_cast<char>(next_c + 0x20)); // +32 для перевода в нижний регистр
                    i++; // Пропускаем следующий байт
                    continue;
                }
            }
            r.push_back(s[i]);
        }
        else if (c == 0xD1) {
            // Первый байт UTF-8 для кириллицы А-Я (продолжение)
            if (i + 1 < s.size()) {
                unsigned char next_c = static_cast<unsigned char>(s[i + 1]);
                if (next_c >= 0x80 && next_c <= 0x8F) { // А-Я (продолжение)
                    r.push_back(static_cast<char>(0xD1));
                    r.push_back(static_cast<char>(next_c + 0x20)); // +32 для перевода в нижний регистр
                    i++; // Пропускаем следующий байт
                    continue;
                }
            }
            r.push_back(s[i]);
        }
        else {
            // Все остальные символы копируем как есть
            r.push_back(s[i]);
        }
    }
    
    return r;
}

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
    
    // Приводим к нижнему регистру все буквы (ICU)
    word = to_lower_utf8_icu(word);
    
    // Проверяем длину
    if (word.length() < 4 || word.length() > 31) {
        std::cout << "[DEBUG] Слово '" << original << "' -> '" << word << "' отфильтровано по длине (" << word.length() << ")" << std::endl;
        return false;
    }
    
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
