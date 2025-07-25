add_executable(client client.cpp)
target_link_libraries(client ipc_shared ws2_32)
set_target_properties(client PROPERTIES EXCLUDE_FROM_ALL TRUE)