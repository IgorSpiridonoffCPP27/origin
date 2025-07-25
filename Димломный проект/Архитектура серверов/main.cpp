#include <thread>
#include <iostream>
#include <cstdlib>

void run(const std::string& cmd) {
    std::system(cmd.c_str());
}

int main() {
    setlocale(LC_ALL, "Rus");
    std::thread([]() { run("bin\\client_server.exe"); }).detach();
    std::thread([]() { run("bin\\data_logger.exe"); }).detach();
    std::thread([]() { run("bin\\monitor.exe"); }).detach();

    std::cout << "Главный сервер запущен. Нажмите Enter для выхода...\n";
    std::cin.get();

    // Для Unix: pkill -f "client_server|data_logger|monitor"
    std::system("taskkill /IM client_server.exe /IM data_logger.exe /IM monitor.exe /F");
    return 0;
}