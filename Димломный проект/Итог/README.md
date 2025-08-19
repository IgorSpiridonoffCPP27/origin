# Дипмломный проект

## Структура проекта
.
├── src/ # Исходные файлы
│ ├── web_proxy.cpp # Основной прокси-сервер
│ ├── crawler.cpp # Краулер для сбора данных
│ ├── DBusers.cpp # Работа с базой данных
│ ├── html_generator.cpp # Генератор HTML интерфейса
│ ├── api_client.cpp # Клиент для API запросов
│ ├── html_parser.cpp # Парсер HTML контента
│ ├── url_tools.cpp # Утилиты для работы с URL
│ ├── logger.cpp # Система логирования
│ └── ... # Дополнительные модули
├── include/ # Заголовочные файлы
├── config/ # Конфигурационные файлы
└── CMakeLists.txt # Файл сборки проекта


## Настройка и запуск

### Предварительные требования
- Компилятор C++17 (GCC, Clang или MSVC)
- Boost 1.70+
- PostgreSQL 12+
- libpqxx 7.0+

### Сборка проекта (Windows, Visual Studio 2022 x64)
```bash
# 1) Чистая конфигурация
rd /s /q build 2>nul & mkdir build

# 2) Генерация под VS 2022 x64 с vcpkg toolchain
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="D:/vcpkg/scripts/buildsystems/vcpkg.cmake"

# 3) Сборка
cmake --build build --config Release

### После сборки запуск .exe 
server.exe
web_proxy.exe
crawler.exe

#Подключаемся к web серверу по адресу localhost:8081