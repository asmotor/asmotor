add_executable (motor6502
    x65_errors.c
    x65_main.c
    x65_options.c
    x65_parse_addrmode.c
    x65_parse_instr_65816.c
    x65_parse_instr.c
    x65_parse.c
    x65_section.c
    x65_symbols.c
    x65_tokens.c
    )

target_link_libraries (motor6502 motor util)

if(NOT MSVC)
    target_link_libraries (motor6502 m)
endif(NOT MSVC)

install (TARGETS motor6502 CONFIGURATIONS Release RUNTIME DESTINATION bin)
