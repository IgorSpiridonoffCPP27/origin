#pragma once

#include "pch.h"   // Для convert_to_utf8()


/**
 * Парсит строку вида "key1=value1&key2=value2" в std::unordered_map.
 * input Входная строка с параметрами.
 * return Словарь ключ-значение.
 */
std::unordered_map<std::string, std::string> extractKeyValues(const std::string& input);

/**
 * Декодирует URL-encoded строку (заменяет %20 на пробелы и т.д.).
 * str Входная строка, например "name%3DJohn%26age%3D30".
 * return Декодированная строка, например "name=John&age=30".
 */
std::string url_decode(const std::string& str);

/**
 * Конвертирует строку в UTF-8 (для кросс-платформенной работы с кодировками).
 * str Входная строка (например, в ANSI или UTF-16).
 * return Строка в UTF-8.
 */
std::string convert_to_utf8(const std::string& str);

/**
 * Устанавливает локаль UTF-8 для корректного вывода в консоль.
 * На Windows также вызывает SetConsoleOutputCP(CP_UTF8).
 */
void set_utf8_locale();