add_executable (motor6809
    m6809_errors.c
    m6809_main.c
    m6809_options.c
    m6809_parse_addrmode.c
    m6809_parse_instr.c
    m6809_parse.c
    m6809_symbols.c
    m6809_tokens.c
    )

target_link_libraries (motor6809 xasm util)

if(NOT MSVC)
    target_link_libraries (motor6809 m)
endif(NOT MSVC)

install (TARGETS motor6809 CONFIGURATIONS Release RUNTIME DESTINATION bin)
