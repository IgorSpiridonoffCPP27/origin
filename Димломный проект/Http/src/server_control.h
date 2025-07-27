#pragma once

#include "pch.h"

/**
 * Глобальный флаг состояния сервера.
 * true - сервер работает, false - требуется остановка.
 * Объявлен как extern, так как определяется в .cpp.
 */
extern std::atomic<bool> server_running;
extern std::atomic<bool> shutdown_complete;
/**
 * Запускает поток для управления сервером.
 * Ожидает ввода 'q' в консоли для graceful shutdown.
 * Выводит инструкцию по остановке сервера.
 */
void server_control_thread();

/**
 * Безопасно останавливает сервер.
 * message Опциональное сообщение для логирования причины остановки.
 */
void graceful_shutdown(const std::string& message = "");