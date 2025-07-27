#include "pch.h"            // Прекомпилированные заголовки (должен быть первым!)
#include "server_control.h"
#include "utils.h"          // Для set_utf8_locale()
#include <iostream>
#include <thread>

// Инициализация глобального флага
std::atomic<bool> server_running(true);

// ================ Реализации функций ================

void server_control_thread() {
    set_utf8_locale();  // Устанавливаем UTF-8 для консоли
    
    std::cout << 
        "Сервер запущен. Для остановки введите 'q' и нажмите Enter...\n";
    
    char input;
    while (server_running.load()) {
        std::cin.get(input);  // Блокирующее чтение
        if (input == 'q') {
            graceful_shutdown("Получена команда остановки");
            break;
        }
    }
}

void graceful_shutdown(const std::string& message) {
    // Атомарно устанавливаем флаг остановки
    bool already_stopped = !server_running.exchange(false);
    
    if (already_stopped) {
        return;  // Сервер уже остановлен
    }
    
    // Логируем причину остановки
    if (!message.empty()) {
        std::cout << "[SHUTDOWN] " << message << std::endl;
    }
    
    // Дополнительные действия при остановке:
    // - Можно добавить закрытие сокетов
    // - Сохранение состояния
    // - Уведомление клиентов
}