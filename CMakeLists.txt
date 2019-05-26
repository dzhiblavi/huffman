cmake_minimum_required(VERSION 3.12)

project(huffman)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -D_GLIBCXX_DEBUG")

add_library(hcoding STATIC encoder.hpp bitset.hpp bitset.cpp util.hpp util.cpp encoder.cpp)

add_executable(hfm huffman.cpp)
add_executable(hfm_test main.cpp)

target_link_libraries(hfm hcoding)
target_link_libraries(hfm_test hcoding)
