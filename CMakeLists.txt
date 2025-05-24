cmake_minimum_required(VERSION 3.13)

project(minesweeper)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

file(GLOB SRC_FILES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB HDR_FILES "${CMAKE_CURRENT_LIST_DIR}/*.h")

add_library(minesweeper STATIC ${HDR_FILE} ${SRC_FILES})
