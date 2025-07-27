cmake_minimum_required(VERSION 3.10)
project(BeastExample)

# Указываем C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Отключаем GNU-расширения (для совместимости)

# Указываем vcpkg (если используется)
set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Ищем Boost и нужные компоненты
find_package(Boost 1.70 REQUIRED COMPONENTS 
    system 
    coroutine
    context
    date_time
)

# Ищем OpenSSL
find_package(OpenSSL REQUIRED)

# Ищем libpqxx
find_package(libpqxx REQUIRED)

# Клиент
add_executable(client src/client_boost.cpp)
target_link_libraries(client PRIVATE 
    Boost::boost
    Boost::system 
    Boost::coroutine
    Boost::context
    Boost::date_time
    OpenSSL::SSL 
    OpenSSL::Crypto
    ws2_32     # Для Windows networking
    crypt32    # Для Windows crypto API
    libpqxx::pqxx
)

# Сервер
add_executable(server
    src/server_boost.cpp 
    src/DBusers.cpp
    src/server_control.cpp
    src/utils.cpp
    src/file_operations.cpp
)

target_link_libraries(server PRIVATE 
    Boost::boost
    Boost::system 
    Boost::coroutine
    Boost::context
    Boost::date_time
    OpenSSL::SSL 
    OpenSSL::Crypto
    ws2_32     # Для Windows networking
    crypt32    # Для Windows crypto API
    libpqxx::pqxx
)

# Тесты (если нужно)
add_executable(test test.cpp)