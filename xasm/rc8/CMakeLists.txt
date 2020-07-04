add_executable (motorrc8
    rc8_errors.c
    rc8_errors.h
    rc8_main.c
    rc8_options.c
    rc8_options.h
    rc8_parse_instr.c
    rc8_parse.c
    rc8_parse.h
    rc8_symbols.c
    rc8_symbols.h
    rc8_tokens.c
    rc8_tokens.h
    )

target_link_libraries (motorrc8 xasm util)

if(NOT MSVC)
    target_link_libraries (motorrc8 m)
endif(NOT MSVC)

install (TARGETS motorrc8 CONFIGURATIONS Release RUNTIME DESTINATION bin)
