cmake_minimum_required(VERSION 3.16)
project(low_latency_event_bus LANGUAGES CXX)

#C++20 
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#Default to Release for Benchmarking
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(Benchmark
    src/main.cpp
)