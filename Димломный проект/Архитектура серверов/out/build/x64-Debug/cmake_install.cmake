# Install script for directory: D:/Курс Разработчик на С++/Базовое программировние в С++/Homework_base_programming/Димломный проект/Архитектура серверов

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/Курс Разработчик на С++/Базовое программировние в С++/Homework_base_programming/Димломный проект/Архитектура серверов/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Курс Разработчик на С++/Базовое программировние в С++/Homework_base_programming/Димломный проект/Архитектура серверов/out/build/x64-Debug/shared/cmake_install.cmake")
  include("D:/Курс Разработчик на С++/Базовое программировние в С++/Homework_base_programming/Димломный проект/Архитектура серверов/out/build/x64-Debug/client_server/cmake_install.cmake")
  include("D:/Курс Разработчик на С++/Базовое программировние в С++/Homework_base_programming/Димломный проект/Архитектура серверов/out/build/x64-Debug/data_logger/cmake_install.cmake")
  include("D:/Курс Разработчик на С++/Базовое программировние в С++/Homework_base_programming/Димломный проект/Архитектура серверов/out/build/x64-Debug/monitor/cmake_install.cmake")
  include("D:/Курс Разработчик на С++/Базовое программировние в С++/Homework_base_programming/Димломный проект/Архитектура серверов/out/build/x64-Debug/client/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "D:/Курс Разработчик на С++/Базовое программировние в С++/Homework_base_programming/Димломный проект/Архитектура серверов/out/build/x64-Debug/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
