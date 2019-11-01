cmake_minimum_required(VERSION 2.8)

#工程名
project(httpserver)

#源码和头文件
aux_source_directory(./src/ SRC_LIST)
include_directories(./inc/)

add_definitions(-g -W -Wall -Wno-deprecated -DLINUX -std=c++11)

set(EXECUTABLE_OUTPUT_PATH ./)

add_executable(httpserver ${SRC_LIST})

target_link_libraries(httpserver pthread)

