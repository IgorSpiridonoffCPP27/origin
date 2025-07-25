cmake_minimum_required(VERSION 3.10)
project(MultiServer)

# Настройка выходных директорий
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Подключаем подпроекты
add_subdirectory(shared)
add_subdirectory(client)
add_subdirectory(client_server)
add_subdirectory(data_logger)
add_subdirectory(monitor)

# Главный сервер (только main.cpp)
add_executable(main_server main.cpp)
target_link_libraries(main_server)

# Копирование общих заголовков
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shared/
     DESTINATION ${CMAKE_BINARY_DIR}/shared)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/shared)