cmake_minimum_required(VERSION 3.10)
project(BeastExample)

# Указываем C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Указываем vcpkg
set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Ищем Boost
find_package(Boost 1.70 REQUIRED COMPONENTS 
    system 
    coroutine
    context
    date_time
    beast 
    url
)

# Ищем OpenSSL
find_package(OpenSSL REQUIRED)

# Ищем libpqxx
find_package(libpqxx REQUIRED)

# Ищем nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)

# Альтернативный способ поиска Gumbo
find_path(GUMBO_INCLUDE_DIR gumbo.h
    PATHS "D:/vcpkg/installed/x64-windows/include"
    REQUIRED
)

find_library(GUMBO_LIBRARY
    NAMES gumbo
    PATHS "D:/vcpkg/installed/x64-windows/lib"
    REQUIRED
)

if(NOT GUMBO_INCLUDE_DIR OR NOT GUMBO_LIBRARY)
    message(FATAL_ERROR "Gumbo-parser not found!")
endif()

add_executable(parsinc parsinc_html.cpp)

target_include_directories(parsinc PRIVATE ${GUMBO_INCLUDE_DIR})

target_link_libraries(parsinc PRIVATE 
    Boost::boost
    Boost::system 
    Boost::coroutine
    Boost::context
    Boost::date_time
    OpenSSL::SSL 
    OpenSSL::Crypto
    Boost::url
    ws2_32
    crypt32
    libpqxx::pqxx
    ${GUMBO_LIBRARY}
)