# logging/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(logging_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# optimizer_testのターゲットを追加
add_executable(optimizer_test tests/optimizer_test.cc)