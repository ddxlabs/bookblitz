cmake_minimum_required(VERSION 3.15...4.1)
project(bookbench VERSION 1 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

# Default build release
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()


# Debug flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -g")

include(FetchContent)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------
option(BUILD_BENCHMARKS "Build profiler" ON)
option(BUILD_TESTS "Build Catch2 unit tests" ON)

# ---------------------------------------------------------------------------
# Make engine library
# ---------------------------------------------------------------------------
add_library(engine STATIC
    src/OrderBook.cpp
    src/Engine.cpp
)


target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(engine PRIVATE -Wall -Wextra)

# ---------------------------------------------------------------------------
# Benchmarks (opt-in; zero production impact)
# ---------------------------------------------------------------------------
if(BUILD_BENCHMARKS)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.1
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googlebenchmark)

    set(BENCH_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bench)
    file(MAKE_DIRECTORY ${BENCH_OUTPUT_DIR})

    add_executable(orderbook_bench tools/bench/orderbook_bench.cpp)

    target_link_libraries(orderbook_bench PRIVATE engine benchmark::benchmark)
    target_compile_options(orderbook_bench PRIVATE -Wall -Wextra)

    set_target_properties(orderbook_bench PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BENCH_OUTPUT_DIR}
    )
endif()

# ---------------------------------------------------------------------------
# Tests (opt-in; zero production impact)
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
    )
    FetchContent_MakeAvailable(Catch2)

    set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/test)

    enable_testing()
    include(Catch)

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS test/*.cpp)

    foreach(TEST_SRC ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_SRC})
        target_link_libraries(${TEST_NAME} PRIVATE engine Catch2::Catch2WithMain)
        set_target_properties(${TEST_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
        )
        catch_discover_tests(${TEST_NAME})
    endforeach()
endif()
