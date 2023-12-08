cmake_minimum_required(VERSION 3.10)
project(lab5-7)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(control.exe ${SOURCE_DIR}/control_node.cpp)
add_executable(calc.exe ${SOURCE_DIR}/calc_node.cpp)
include_directories(${INCLUDE_DIR})

find_package(cppzmq REQUIRED)
target_link_libraries(control.exe cppzmq)
target_link_libraries(calc.exe cppzmq)