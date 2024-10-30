# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(logging_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Manager test
add_executable(manager_test tests/manager_test.cc)

# Optimizer test
add_executable(optimizer_test tests/optimizer_test.cc)