cmake_minimum_required(VERSION 3.1)
project(qhm)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)
include_directories(lib)

include_directories(src)

add_subdirectory(lib)
add_subdirectory(src)

enable_testing()
add_subdirectory(test)

