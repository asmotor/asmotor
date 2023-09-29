add_executable (motorrc8
    rc8_errors.c
    rc8_main.c
    rc8_options.c
    rc8_parse_instr.c
    rc8_parse.c
    rc8_section.c
    rc8_symbols.c
    rc8_tokens.c
    )

target_link_libraries (motorrc8 motor util)
target_include_directories (motorrc8 PRIVATE "${PROJECT_SOURCE_DIR}/xasm/motor")

if(NOT MSVC)
    target_link_libraries (motorrc8 m)
endif(NOT MSVC)

install (TARGETS motorrc8 CONFIGURATIONS Release RUNTIME DESTINATION bin)
