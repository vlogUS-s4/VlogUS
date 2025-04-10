cmake_minimum_required(VERSION 3.10)
project(MyProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Specify include directories (optional if in same directory)
include_directories(${CMAKE_SOURCE_DIR})

# Add executable
add_executable(${PROJECT_NAME} main.cpp)
