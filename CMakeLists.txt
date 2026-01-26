cmake_minimum_required(VERSION 3.5)
project(minico C ASM)

set(CMAKE_C_STANDARD 11)

find_package(Threads REQUIRED)

message(STATUS "CMAKE_THREAD_LIBS_INIT: ${CMAKE_THREAD_LIBS_INIT}")
message(STATUS "CMAKE_USE_PTHREADS_INIT: ${CMAKE_USE_PTHREADS_INIT}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_subdirectory(test)
add_subdirectory(src)


enable_testing()
