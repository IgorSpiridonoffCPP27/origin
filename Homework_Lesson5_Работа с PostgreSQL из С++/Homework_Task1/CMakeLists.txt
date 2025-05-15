cmake_minimum_required(VERSION 3.20)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)

# Путь к распакованному libpqxx
set(LIBPQXX_DIR "D:/libs/libpqxx")

# Указываем путь к PostgreSQL
set(PostgreSQL_INCLUDE_DIR "C:/Program Files/PostgreSQL/17/include")
set(PostgreSQL_LIBRARY "C:/Program Files/PostgreSQL/17/lib/libpq.lib")

# Добавляем libpqxx
add_subdirectory(${LIBPQXX_DIR} ${LIBPQXX_DIR}/build)

add_executable(MyProject main.cpp)

target_include_directories(MyProject PRIVATE ${PostgreSQL_INCLUDE_DIR})
target_link_libraries(MyProject PRIVATE pqxx ${PostgreSQL_LIBRARY})
