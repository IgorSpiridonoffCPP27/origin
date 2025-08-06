cmake_minimum_required(VERSION 3.10)
project(test_boost)

find_package(Boost REQUIRED COMPONENTS algorithm)

add_executable(test_boost main.cpp)
target_link_libraries(test_boost PRIVATE Boost::boost)