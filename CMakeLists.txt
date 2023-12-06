cmake_minimum_required(VERSION 3.10)
project(lab5-7)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(main.exe main.cpp)
include_directories(${INCLUDE_DIR})

find_package(cppzmq REQUIRED)
target_link_libraries(main.exe cppzmq)