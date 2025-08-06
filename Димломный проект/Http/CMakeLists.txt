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

#Ищем nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)

# Клиент1
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

# Сервер1
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


# Клиент2
add_executable(client2 src2/client_boost.cpp)
target_link_libraries(client2 PRIVATE 
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
    nlohmann_json::nlohmann_json
)

# Сервер2
add_executable(server2
    src2/server_boost.cpp 
    src2/DBusers.cpp
    src2/server_control.cpp
    src2/utils.cpp
    src2/file_operations.cpp
)

target_link_libraries(server2 PRIVATE 
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
    nlohmann_json::nlohmann_json
)

# Тесты (если нужно)
add_executable(test test.cpp)