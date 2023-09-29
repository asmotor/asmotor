add_executable(motorz80
        z80_errors.c
        z80_main.c
        z80_options.c
        z80_parse.c
        z80_section.c
        z80_symbols.c
        z80_tokens.c
        )

target_link_libraries(motorz80 motor util)
target_include_directories (motorz80 PRIVATE "${PROJECT_SOURCE_DIR}/xasm/motor")

if(NOT MSVC)
    target_link_libraries (motorz80 m)
endif(NOT MSVC)

install (TARGETS motorz80 CONFIGURATIONS Release RUNTIME DESTINATION bin)
