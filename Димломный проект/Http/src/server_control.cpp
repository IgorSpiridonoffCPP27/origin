#include "pch.h"
#include "server_control.h"
#include "utils.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <stdlib.h> // Для exit()

std::atomic<bool> server_running(true);
std::atomic<bool> shutdown_complete(false);

void server_control_thread() {
    set_utf8_locale();
    
    std::cout << "Server running. Press 'q'+Enter to stop...\n";
    
    char input;
    while (server_running) {
        std::cin.get(input);
        if (input == 'q') {
            graceful_shutdown("Shutdown command received");
            break;
        }
        while (std::cin.get() != '\n' && server_running);
    }
}

void graceful_shutdown(const std::string& message) {
    if (!server_running.exchange(false)) return;
    
    if (!message.empty()) {
        std::cout << "\n[SHUTDOWN] " << message << "\n";
    }
    
    const int countdown_seconds = 5;
    std::cout << "Window will close in: ";
    for (int i = countdown_seconds; i > 0; --i) {
        std::cout << i << "... " << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "0. Shutting down.\n";
    shutdown_complete = true;
    
    // Явное завершение программы
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    exit(0);
}