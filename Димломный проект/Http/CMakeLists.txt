cmake_minimum_required(VERSION 3.10)
project(BeastExample)

# Указываем vcpkg
set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Ищем Boost и нужные компоненты
find_package(Boost REQUIRED COMPONENTS 
    system 
    coroutine  # Необходимо для Boost.Asio
    context    # Необходимо для Boost.Asio
    date_time  # Может потребоваться для некоторых функций
)

# Ищем OpenSSL
find_package(OpenSSL REQUIRED)

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
)

# Сервер
add_executable(server server_boost.cpp)
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
)


add_executable(test test.cpp)