cmake_minimum_required(VERSION 3.10)
project(pr6 C)

set(CMAKE_C_STANDARD 11)

find_package(Threads REQUIRED)

add_executable(t1 src/t1.c)
target_link_libraries(t1 Threads::Threads)

add_executable(t2 src/t2.c)
target_link_libraries(t2 Threads::Threads)
