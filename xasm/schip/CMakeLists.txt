add_executable (motorschip 
    schip_error.c
    schip_main.c
    schip_options.c
    schip_parse_instr.c
    schip_parse.c
    schip_section.c
    schip_symbols.c
    schip_tokens.c
    )

target_link_libraries (motorschip motor util)

if(NOT MSVC)
    target_link_libraries (motorschip m)
endif(NOT MSVC)

install (TARGETS motorschip CONFIGURATIONS Release RUNTIME DESTINATION bin)
