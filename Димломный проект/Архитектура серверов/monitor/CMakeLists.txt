add_executable(monitor monitor.cpp)
target_link_libraries(monitor ipc_shared ws2_32)
set_target_properties(monitor PROPERTIES EXCLUDE_FROM_ALL TRUE)