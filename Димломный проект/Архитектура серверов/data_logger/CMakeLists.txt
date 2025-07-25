add_executable(data_logger data_logger.cpp)
target_include_directories(data_logger PRIVATE ../shared)
target_link_libraries(data_logger ipc_shared ws2_32)

if(WIN32)
    set_target_properties(data_logger PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()