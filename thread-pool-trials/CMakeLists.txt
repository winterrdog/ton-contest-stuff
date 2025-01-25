cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(thread_pool_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED)

add_executable(test_pool test-pool.cc)

# Link against pthread
target_link_libraries(test_pool
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread)