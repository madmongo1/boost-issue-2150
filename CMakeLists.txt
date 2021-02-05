cmake_minimum_required(VERSION 3.18)
project(untitled1)

set(CMAKE_CXX_STANDARD 20)

add_executable(untitled1 main.cpp)

find_package(Threads)
target_link_libraries(untitled1 Threads::Threads)
