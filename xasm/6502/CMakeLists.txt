add_executable (motor6502
    x65_errors.c
    x65_errors.h
    x65_main.c
    x65_options.c
    x65_options.h
    x65_parse_addrmode.c
    x65_parse_instr.c
    x65_parse.c
    x65_parse.h
    x65_symbols.c
    x65_tokens.c
    x65_tokens.h
    )

target_link_libraries (motor6502 xasm util)

if(NOT MSVC)
    target_link_libraries (motor6502 m)
endif(MSVC)

install (TARGETS motor6502 CONFIGURATIONS Release RUNTIME DESTINATION bin)
