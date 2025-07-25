add_library(ipc_shared STATIC ipc.cpp)
target_include_directories(ipc_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ipc_shared ws2_32)