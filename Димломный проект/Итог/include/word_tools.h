#pragma once
#include <string>
// Фильтрует и нормализует слово: приводит к нижнему регистру, удаляет кавычки, триммингует. Возвращает true, если слово подходит (4-31 символ, без кавычек)
bool filter_and_normalize_word(std::string& word);
