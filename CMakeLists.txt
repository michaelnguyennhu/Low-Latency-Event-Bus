
cmake_minimum_required(VERSION 3.16)
project(low_latency_event_bus LANGUAGES CXX)


# --- Global C++ Settings -----------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release for Benchmarking; can override with -DCMAKE_BUILD_TYPE=Debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

#Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Main Benchmark executable -----------------------------------------------------

add_executable(benchmark
    src/main.cpp
    src/event_bus.cpp
    src/latency_tracker.cpp
)

# Tell the target where to find our headers - in include/...
target_include_directories(benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Compiler Warnings and Optimizations 
if (MSVC) 
    target_compile_options(benchmark PRIVATE
        /O2.                # Optimize for Speed
        /W4                 # High Warning Level
        /permissive-        # Stricter C++ Conformance
    )
else() 
    target_compile_options(benchmark PRIVATE
        -O3                 # Aggressive Optimization (For Benchmarking)
        -march=native       # Use all CPU features on the Machine
        -Wall               # All common warnings
        -Wextra             # More Warnings
        -Wpedantic          # Strict ISO C++ Warnings
    )
endif()

# Link pthread on Linux-like systems (needed for std::thread)
if (UNIX AND NOT APPLE)
    target_link_libraries(benchmark PRIVATE pthread)
endif()


# --- GoogleTest Setup (fetched via FetchContent) -----------------------------------

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

# On MSVC, make sure gtest uses the same runtime as the rest of the project
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Enable CTest Integration (ctest command) 
enable_testing() 


# --- Test Executables --------------------------------------------------------------

add_executable(tests
    tests/test_ring_buffer.cpp
    tests/test_latency_tracker.cpp
    src/latency_tracker.cpp         #reuse latency_tracker implementation
)

target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(tests PRIVATE
    GTest::gtest_main
)

# pthread for tests as well, if needed
if (UNIX AND NOT APPLE)
    target_link_libraries(tests PRIVATE pthread)
endif()

#register the tests with CTest: 'ctest' will run this executable
add_test(NAME all_tests COMMAND tests)