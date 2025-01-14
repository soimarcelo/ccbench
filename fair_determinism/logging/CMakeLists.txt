cmake_minimum_required(VERSION 3.10)
project(logging_optimization VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

# 実行ファイルの追加
add_executable(experiment1_test tests/experiment1_test.cc)
add_executable(optimizer_test tests/optimizer_test.cc)
add_executable(optimizer_test_large tests/optimizer_test_large.cc)
add_executable(experiment2_test tests/experiment2_test.cc)  # rw_ratio_testの代わりに追加

# スレッドライブラリのリンク
target_link_libraries(experiment1_test PRIVATE Threads::Threads)
target_link_libraries(optimizer_test PRIVATE Threads::Threads)
target_link_libraries(optimizer_test_large PRIVATE Threads::Threads)
target_link_libraries(experiment2_test PRIVATE Threads::Threads)  # rw_ratio_testの代わりに追加