cmake_minimum_required(VERSION 3.10)
project(BeastExample)

# Указываем C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Указываем vcpkg
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

# Ищем nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)

# Ручной поиск Gumbo
# Замените базовый путь на ваш
set(VCPKG_BASE_DIR "D:/vcpkg/installed/x64-windows")

find_path(GUMBO_INCLUDE_DIR NAMES gumbo.h
    PATHS "${VCPKG_BASE_DIR}/include"
    NO_DEFAULT_PATH
)

find_library(GUMBO_LIBRARY_RELEASE NAMES gumbo.lib
    PATHS "${VCPKG_BASE_DIR}/lib"
    NO_DEFAULT_PATH
)

find_library(GUMBO_LIBRARY_DEBUG NAMES gumbo.lib
    PATHS "${VCPKG_BASE_DIR}/debug/lib"
    NO_DEFAULT_PATH
)

if(NOT GUMBO_INCLUDE_DIR)
    message(FATAL_ERROR "Gumbo include directory not found")
endif()

if(NOT GUMBO_LIBRARY_RELEASE OR NOT GUMBO_LIBRARY_DEBUG)
    message(FATAL_ERROR "Gumbo libraries not found")
endif()

# Создаем интерфейсную цель для Gumbo
add_library(gumbo INTERFACE)
target_include_directories(gumbo INTERFACE ${GUMBO_INCLUDE_DIR})
target_link_libraries(gumbo INTERFACE
    $<$<CONFIG:Debug>:${GUMBO_LIBRARY_DEBUG}>
    $<$<NOT:$<CONFIG:Debug>>:${GUMBO_LIBRARY_RELEASE}>
)

# crowler
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
    ws2_32
    crypt32
    libpqxx::pqxx
    gumbo
)

# Клиент1
add_executable(client src/client.cpp)
target_link_libraries(client PRIVATE 
    Boost::boost
    Boost::system 
    Boost::coroutine
    Boost::context
    Boost::date_time
    OpenSSL::SSL 
    OpenSSL::Crypto
    ws2_32
    crypt32
    libpqxx::pqxx
)

# Сервер1
add_executable(server
    src/server.cpp 
    src/DBusers.cpp
)

target_link_libraries(server PRIVATE 
    Boost::boost
    Boost::system 
    Boost::coroutine
    Boost::context
    Boost::date_time
    OpenSSL::SSL 
    OpenSSL::Crypto
    ws2_32
    crypt32
    libpqxx::pqxx
)

# Веб Сервер
add_executable(web_server
    src/web_proxy.cpp 
    src/DBusers.cpp
)

target_link_libraries(web_server PRIVATE 
    Boost::boost
    Boost::system 
    Boost::coroutine
    Boost::context
    Boost::date_time
    OpenSSL::SSL 
    OpenSSL::Crypto
    ws2_32
    crypt32
    libpqxx::pqxx
)