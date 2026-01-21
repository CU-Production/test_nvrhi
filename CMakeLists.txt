cmake_minimum_required(VERSION 4.0)
project(test_nvrhi)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(3rd_party)

add_executable(test_nvrhi main.cpp)
