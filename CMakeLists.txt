cmake_minimum_required(VERSION 3.18)
project(minico C)

set(CMAKE_C_STANDARD 11)

find_package(Threads REQUIRED)

message(STATUS "CMAKE_THREAD_LIBS_INIT: ${CMAKE_THREAD_LIBS_INIT}")
message(STATUS "CMAKE_USE_PTHREADS_INIT: ${CMAKE_USE_PTHREADS_INIT}")

add_subdirectory(test)
add_subdirectory(src)


enable_testing()
