cmake_minimum_required(VERSION 3.10)

# Политика для Boost в CMake 4: используем CONFIG-пакеты
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Подключаем vcpkg toolchain ДО вызова project(), если его не передали через -D
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "Vcpkg toolchain file")
endif()

project(BeastExample)

# Указываем C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: устраняем конфликты PDB
if(MSVC)
    add_compile_options(/FS)
    # Встраиваем отладочную информацию в OBJ (/Z7), чтобы исключить гонки за общий PDB
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
endif()

# Подсказка путей для поиска CONFIG-пакетов vcpkg
if(DEFINED ENV{VCPKG_ROOT})
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows")
endif()
list(PREPEND CMAKE_PREFIX_PATH "D:/vcpkg/installed/x64-windows")

# Ищем Boost и нужные компоненты (через vcpkg config)
find_package(Boost 1.70 CONFIG REQUIRED COMPONENTS
    system
    coroutine
    context
    date_time
)
find_package(ICU COMPONENTS uc i18n dt REQUIRED)

# Ищем OpenSSL
find_package(OpenSSL REQUIRED)

# Ищем libpqxx
find_package(libpqxx REQUIRED)

# Ищем nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)

# Добавляем путь для поиска заголовков
include_directories(include)

# Ручной поиск Gumbo
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

# crowler - обновленный список файлов
add_executable(crowler 
    src/crowler.cpp
    src/crawler.cpp
    src/crawler_config.cpp
    src/DBusers.cpp
    src/logger.cpp
    src/html_parser.cpp
    src/threadsafe_set.cpp
    src/url_tools.cpp
    src/word_tools.cpp
    
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
    nlohmann_json::nlohmann_json  # Добавлено, если используется JSON
    ${ICU_LIBRARIES}
    ICU::uc
    ICU::i18n
)

# Подключаем заголовки ICU для целей, где используется word_tools
target_include_directories(crowler PRIVATE ${ICU_INCLUDE_DIRS})

if(MSVC)
    set_target_properties(crowler PROPERTIES 
        COMPILE_PDB_NAME "crowler"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb/crowler"
    )
    # Явно указываем отдельный PDB для компилятора, чтобы не использовать общий vc143.pdb
    target_compile_options(crowler PRIVATE 
        /Fd"${CMAKE_BINARY_DIR}/pdb/crowler/crowler_compile.pdb"
    )
endif()


# Сервер1 (без изменений)
add_executable(server
    src/server.cpp 
    src/DBusers.cpp
    src/word_tools.cpp
    src/url_tools.cpp
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
    ${ICU_LIBRARIES}
    ICU::uc
    ICU::i18n
)

target_include_directories(server PRIVATE ${ICU_INCLUDE_DIRS})

if(MSVC)
    set_target_properties(server PROPERTIES 
        COMPILE_PDB_NAME "server"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb/server"
    )
    target_compile_options(server PRIVATE 
        /Fd"${CMAKE_BINARY_DIR}/pdb/server/server_compile.pdb"
    )
endif()

# Веб Сервер (без изменений)
add_executable(web_server
    src/web_proxy.cpp 
    src/api_client.cpp
    src/proxy_config.cpp
    src/html_generator.cpp
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
    ${ICU_LIBRARIES}
)

if(MSVC)
    set_target_properties(web_server PROPERTIES 
        COMPILE_PDB_NAME "web_server"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb/web_server"
    )
    target_compile_options(web_server PRIVATE 
        /Fd"${CMAKE_BINARY_DIR}/pdb/web_server/web_server_compile.pdb"
    )
endif()
