cmake_minimum_required(VERSION 3.10)
project(MultiServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Для копирования файлов в нужную папку
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)


# Для поддержки кириллицы в путях (Windows)
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_subdirectory(shared)
add_subdirectory(client_server)
add_subdirectory(data_logger)
add_subdirectory(monitor)
add_subdirectory(client)

add_executable(main_server main.cpp)

if(MSVC)
    add_compile_options(/Zc:wchar_t-)
endif()