cmake_minimum_required (VERSION 3.10)

include (ucm.cmake)

set (CMAKE_C_STANDARD 99)

if(MSVC)
  add_compile_options(/W3 /WX /wd4100 /D_CRT_SECURE_NO_WARNINGS)
else(MSVC)
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter)
endif(MSVC)

ucm_set_runtime (STATIC)

include_directories ("${PROJECT_SOURCE_DIR}/util")

add_subdirectory (util)
add_subdirectory (xasm)
add_subdirectory (xlib)
add_subdirectory (xlink)
