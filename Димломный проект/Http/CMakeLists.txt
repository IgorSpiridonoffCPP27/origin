cmake_minimum_required(VERSION 3.10)
project(BeastExample)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)


# Указываем vcpkg
set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Ищем Boost и нужные компоненты
find_package(Boost REQUIRED COMPONENTS 
    system 
    coroutine  # Необходимо для Boost.Asio
    context    # Необходимо для Boost.Asio
    date_time  # Может потребоваться для некоторых функций
)

# Ищем OpenSSL
find_package(OpenSSL REQUIRED)

# Ищем libpqxx
find_package(libpqxx REQUIRED)

# Клиент
add_executable(client client_boost.cpp)
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
    src/server server_boost.cpp 
    src/DBusers.cpp
    src/utils.cpp
    src/server_control.cpp
    src/file_operations.cpp
)

target_precompile_headers(server PRIVATE
    src/pch.h
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



#Для тестов
add_executable(test test.cpp)