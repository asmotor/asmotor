include(ExternalProject)

ExternalProject_Add(token_generator
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/xasm//token_generator"
)

include_directories ("${PROJECT_SOURCE_DIR}/xasm/motor")

add_subdirectory (motor)
add_subdirectory (6502)
add_subdirectory (6809)
add_subdirectory (680x0)
add_subdirectory (dcpu-16)
add_subdirectory (mips)
add_subdirectory (rc8)
add_subdirectory (schip)
add_subdirectory (z80)

