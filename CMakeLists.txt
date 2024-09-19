cmake_minimum_required (VERSION 3.10)

project(ASMotor)

include (build/ucm.cmake)

set (CMAKE_C_STANDARD 99)

if(MSVC)
  add_compile_options(/W3 /WX /wd4068 /J /D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1)
else(MSVC)
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter -funsigned-char)
endif(MSVC)

if(NOT (${CMAKE_SYSTEM_NAME} STREQUAL Darwin))
  link_libraries("-static")
endif()

ucm_set_runtime(STATIC)

add_definitions(-DASMOTOR_VERSION="${ASMOTOR_VERSION}")

include_directories ("${PROJECT_SOURCE_DIR}/util")

add_subdirectory (util)
add_subdirectory (xasm)
add_subdirectory (xlib)
add_subdirectory (xlink)
