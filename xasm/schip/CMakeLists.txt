add_executable (motorschip 
    schip_error.c
    schip_error.h
    schip_main.c
    schip_options.c
    schip_options.h
    schip_parse_instr.c
    schip_parse.c
    schip_parse.h
    schip_symbols.c
    schip_symbols.h
    schip_tokens.c
    schip_tokens.h
    )

target_link_libraries (motorschip xasm util m)
install (TARGETS motorschip CONFIGURATIONS Release RUNTIME DESTINATION bin)
