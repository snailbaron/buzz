cmake_minimum_required(VERSION 3.0)

project(buzz)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(deps)

add_subdirectory(src)
