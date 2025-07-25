#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

void launch(const std::string& exe_name) {
    // Получаем абсолютный путь к исполняемому файлу
    fs::path exe_path = fs::absolute("bin/" + exe_name);

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    std::string cmd = "\"" + exe_path.string() + "\"";

    if (!CreateProcess(
        NULL,
        &cmd[0], // Преобразуем в mutable char*
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL, // Рабочая директория = директория main_server.exe
        &si,
        &pi
    )) {
        DWORD error = GetLastError();
        std::cerr << "Failed to start " << exe_name
            << ". Error code: " << error << "\n";

        // Вывод расшифровки ошибки
        LPSTR message = nullptr;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            error,
            0,
            (LPSTR)&message,
            0,
            NULL
        );
        std::cerr << "System message: " << message << "\n";
        LocalFree(message);
    }
    else {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        std::cout << "Successfully started " << exe_name << "\n";
    }
}

int main() {
    // Проверяем существование bin/
    if (!fs::exists("bin")) {
        fs::create_directory("bin");
    }

    // Проверяем существование EXE-файлов
    for (const auto& exe : { "client_server.exe", "data_logger.exe", "monitor.exe" }) {
        if (!fs::exists("bin/" + std::string(exe))) {
            std::cerr << "Error: " << exe << " not found in bin/\n";
            return 1;
        }
    }

    // Запуск компонентов
    std::thread([]() { launch("client_server.exe"); }).detach();
    std::thread([]() { launch("data_logger.exe"); }).detach();
    std::thread([]() { launch("monitor.exe"); }).detach();

    std::cout << "Main server running. Press Enter to exit...\n";
    std::cin.get();

    // Завершение процессов
    system("taskkill /IM client_server.exe /F >nul 2>&1");
    system("taskkill /IM data_logger.exe /F >nul 2>&1");
    system("taskkill /IM monitor.exe /F >nul 2>&1");

    return 0;
}