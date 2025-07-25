add_executable(monitor monitor.cpp)
target_include_directories(monitor PRIVATE ../shared)
target_link_libraries(monitor ipc_shared ws2_32)

if(WIN32)
    set_target_properties(monitor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()