add_executable (motor68k
    m68k_errors.c
    m68k_errors.h
    m68k_options.c
    m68k_options.h
    m68k_main.c
    m68k_parse.c
    m68k_parse_addrmode.c
    m68k_parse_directive.c
    m68k_parse_fpu.c
    m68k_parse_integer.c
    m68k_symbols.c
    m68k_tokens.c
    m68k_tokens.h
    )

target_link_libraries (motor68k xasm util)
install (TARGETS motor68k CONFIGURATIONS Release RUNTIME DESTINATION bin)
