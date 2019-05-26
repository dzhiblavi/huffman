cmake_minimum_required(VERSION 3.12)

project(huffman)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pthread -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -D_GLIBCXX_DEBUG")

#add_library(hcoding STATIC)
add_executable(huffman
        main.cpp
#        huffman.cpp
        encoder.hpp bitset.hpp bitset.cpp util.hpp util.cpp encoder.cpp)
