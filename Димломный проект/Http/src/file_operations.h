#pragma once

#include "pch.h" 

// Forward declaration для класса БД
class DBuse;

namespace http = boost::beast::http;

/**
 * Записывает данные в лог-файл с временной меткой.
 * param data Строка для записи (автоматически конвертируется в UTF-8).
 */
void save_to_log(const std::string& data);

/**
 * Читает HTML-шаблон главной страницы.
 * return Содержимое файла main.html.
 * throws std::runtime_error Если файл не найден.
 */
std::string read_html_main();

/**
 * Читает HTML-шаблон для POST-ответа.
 * @return Содержимое файла POST_log.html или заглушку при ошибке.
 */
std::string read_post_template();

/**
 * Обрабатывает HTTP-запрос и формирует ответ.
 * @param req Входящий HTTP-запрос.
 * @param res Исходящий HTTP-ответ (заполняется внутри функции).
 * @param connectiondb Соединение с БД для сохранения данных.
 */
void handle_request(
    http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    DBuse& connectiondb
);