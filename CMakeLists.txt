cmake_minimum_required (VERSION 3.10)

project(ASMotor)

include (build/ucm.cmake)

set (CMAKE_C_STANDARD 99)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

include(CTest)
add_test (NAME test_patch_error_paths COMMAND test_patch_error_paths WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xasm/test)
add_test (NAME test_include_paths COMMAND test_include_paths WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xasm/test)
add_test (NAME test_dependency_paths COMMAND test_dependency_paths WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xasm/test)

# Integration tests for each backend
# Note: gameboy uses z80 assembler with -mcg flag
set (GAMEBOY_BUILD_DIR ${CMAKE_BINARY_DIR}/xasm/z80)
foreach (backend 6502 6809 dcpu-16 mips rc8 schip z80)
    add_test (NAME test_integration_${backend}
              COMMAND test_runner
                      --test-dir ${PROJECT_SOURCE_DIR}/test
                      --build-dir ${CMAKE_BINARY_DIR}/xasm/${backend}
                      --suite ${backend}
              WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xasm/test)
    set_tests_properties (test_integration_${backend} PROPERTIES TIMEOUT 30)
endforeach ()
add_test (NAME test_integration_gameboy
          COMMAND test_runner
                  --test-dir ${PROJECT_SOURCE_DIR}/test
                  --build-dir ${GAMEBOY_BUILD_DIR}
                  --suite gameboy
          WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xasm/test)
set_tests_properties (test_integration_gameboy PROPERTIES TIMEOUT 30)
# 680x0 and common use original shell scripts (special formats)
add_test (NAME test_integration_680x0
          COMMAND sh run.sh
          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/680x0)
set_tests_properties (test_integration_680x0 PROPERTIES TIMEOUT 30)

add_test (NAME test_integration_common
          COMMAND sh run.sh
          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/common)
set_tests_properties (test_integration_common PROPERTIES TIMEOUT 60)
