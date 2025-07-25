#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

void launch(const wchar_t* exe_name) {
    // Получаем путь к текущему исполняемому файлу
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    fs::path current_path = fs::path(path).parent_path();
    
    // Ищем в build/bin/Release
    fs::path exe_path = current_path.parent_path().parent_path() / "bin" / "Release" / exe_name;

    // Проверяем существование файла
    if (!fs::exists(exe_path)) {
        std::wcerr << L"Error: " << exe_path.wstring() << L" not found!\n";
        return;
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    std::wstring cmd = L"\"" + exe_path.wstring() + L"\"";

    if (!CreateProcessW(
        NULL,
        &cmd[0],
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        current_path.wstring().c_str(),
        &si,
        &pi
    )) {
        DWORD error = GetLastError();
        std::wcerr << L"Failed to start " << exe_name 
                  << L". Error code: " << error << L"\n";
    } else {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        std::wcout << L"Successfully started " << exe_name << L"\n";
    }
}

int main() {
    // Получаем путь к исполняемым файлам
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    fs::path bin_path = fs::path(path).parent_path().parent_path().parent_path() / "bin" / "Release";

    std::wcout << L"Ищем EXE в: " << bin_path.wstring() << std::endl;

    const wchar_t* executables[] = {
        L"client_server.exe",
        L"data_logger.exe",
        L"monitor.exe"
    };

    // Проверяем наличие всех EXE-файлов
    for (const auto& exe : executables) {
        if (!fs::exists(bin_path / exe)) {
            std::wcerr << L"Critical error: " << exe 
                      << L" not found in " << bin_path.wstring() << L"\n";
            return 1;
        }
    }

    // Запуск компонентов
    std::thread([](){ launch(L"client_server.exe"); }).detach();
    std::thread([](){ launch(L"data_logger.exe"); }).detach();
    std::thread([](){ launch(L"monitor.exe"); }).detach();

    std::wcout << L"Main server running. Press Enter to exit...\n";
    std::cin.get();

    // Завершение процессов
    system("taskkill /IM client_server.exe /F >nul 2>&1");
    system("taskkill /IM data_logger.exe /F >nul 2>&1");
    system("taskkill /IM monitor.exe /F >nul 2>&1");

    return 0;
}