cmake_minimum_required(VERSION 4.0)
project(test_nvrhi)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build third-party libraries
add_subdirectory(3rd_party)

# Build common library (device management abstraction)
add_subdirectory(src/common)

# Build demos
add_subdirectory(src/triangle)
