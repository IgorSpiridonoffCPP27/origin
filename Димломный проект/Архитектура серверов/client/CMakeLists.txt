add_executable(client client.cpp)
target_include_directories(client PRIVATE ../shared)
target_link_libraries(client ipc_shared ws2_32)

if(WIN32)
    set_target_properties(client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()