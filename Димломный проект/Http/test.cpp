#include <iostream>
#include <fstream>
#include <filesystem>
#include <windows.h>
namespace fs = std::filesystem;

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    fs::path path = "../../test.txt";  // Относительный путь
    std::ifstream file(path);

    if (file) {
        std::cout << "Файл открыт: " << fs::absolute(path) << std::endl;
    } else {
        std::cerr << "Ошибка! Проверьте путь: " << fs::absolute(path) << std::endl;
    }

    system("pause");
}