add_executable (motor68k
    m68k_errors.c
    m68k_options.c
    m68k_main.c
    m68k_parse.c
    m68k_parse_addrmode.c
    m68k_parse_directive.c
    m68k_parse_fpu.c
    m68k_parse_integer.c
    m68k_section.c
    m68k_symbols.c
    m68k_tokens.c
    )

target_link_libraries (motor68k xasm util)

if(NOT MSVC)
    target_link_libraries (motor68k m)
endif(NOT MSVC)

install (TARGETS motor68k CONFIGURATIONS Release RUNTIME DESTINATION bin)
