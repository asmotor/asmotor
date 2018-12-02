cmake_minimum_required(VERSION 3.12)

set (CMAKE_C_STANDARD 99)

include_directories ("${PROJECT_SOURCE_DIR}/util")

add_subdirectory (util)
add_subdirectory (xasm)
add_subdirectory (xgbfix)
add_subdirectory (xlib)
add_subdirectory (xlink)