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
add_executable(crowler 
src/crowler.cpp
src/DBusers.cpp
)
target_link_libraries(crowler PRIVATE 
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

