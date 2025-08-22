# Краулер для анализа веб-страниц

## Описание проекта

Проект представляет собой систему для краулинга веб-страниц, анализа текстового содержимого и индексации слов в базе данных PostgreSQL. Система состоит из трех основных компонентов:

1. **crowler** - основной краулер для обхода веб-страниц
2. **server** - сервер API для работы с базой данных  
3. **web_server** - веб-прокси с генерацией HTML

## Архитектура

- **C++20** - основной язык разработки
- **Boost.Asio** - асинхронный I/O
- **Boost.Beast** - HTTP/HTTPS
- **PostgreSQL** - база данных
- **Gumbo** - парсинг HTML
- **OpenSSL** - SSL/TLS
- **ICU** - Unicode поддержка

## Структура проекта

```
├── include/           # Заголовочные файлы
├── src/              # Исходные файлы
├── config.ini        # Конфигурация
├── CMakeLists.txt    # Система сборки
└── README.md         # Документация
```

## Зависимости

- Boost 1.70+ (system, coroutine, context, date_time)
- OpenSSL
- libpqxx (PostgreSQL)
- Gumbo (HTML парсер)
- ICU (Unicode)
- nlohmann/json (JSON)

## Сборка

### Требования
- CMake 3.10+
- Visual Studio 2019+ (Windows)
- vcpkg с установленными зависимостями

### Команды сборки
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="D:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build . --config Release
```

## Конфигурация

Отредактируйте `config.ini` для настройки:

```ini
[Database]
host=localhost
port=5432
dbname=HTTP
user=postgres
password=your_password

[Crowler]
start_page=https://example.com
max_redirects=5
recursion_depth=1
max_connections=10
max_links_per_page=1
poll_interval=5

[server]
port=8080

[DBusers]
POOL_SIZE=5
```

## Использование

1. **Настройте базу данных PostgreSQL**
2. **Отредактируйте config.ini**
3. **Запустите компоненты:**

```bash
# Краулер
./Release/crowler.exe

# API сервер  
./Release/server.exe

# Веб-прокси
./Release/web_server.exe
```

## Функциональность

### Краулер
- Рекурсивный обход веб-страниц
- Извлечение и индексация слов
- Сохранение HTML и текстового содержимого
- Фильтрация и нормализация слов

### API Сервер
- REST API для работы с базой данных
- Поиск по словам и URL
- Статистика и метаданные

### Веб-прокси
- Прокси-сервер с генерацией HTML
- Веб-интерфейс для просмотра результатов

## Подробная документация

См. файл `PROJECT_DESCRIPTION.md` для детального описания каждого компонента и файла проекта.

## Лицензия

Проект разработан в рамках дипломной работы по программированию на C++.