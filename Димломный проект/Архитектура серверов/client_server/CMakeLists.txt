add_executable(client_server client_server.cpp)
target_include_directories(client_server PRIVATE ../shared)
target_link_libraries(client_server ipc_shared ws2_32)

# Для Windows установите выходную директорию
if(WIN32)
    set_target_properties(client_server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()