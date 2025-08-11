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

### Сборка проекта
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="D:/vcpkg/scripts/buildsystems/vcpkg.cmake" #так как файл vcpkg лежит в нестандартном пути
cd build && cmake --build .

### После сборки запуск .exe 
server.exe
web_proxy.exe
crawler.exe

#Подключаемся к web серверу по адресу localhost:8081