add_executable(motorz80
        z80_errors.c
        z80_errors.h
        z80_main.c
        z80_options.c
        z80_options.h
        z80_parse.c
        z80_parse.h
        z80_symbols.c
        z80_symbols.h
        z80_tokens.c
        z80_tokens.h
        )

target_link_libraries(motorz80 xasm util)
install (TARGETS motorz80 CONFIGURATIONS Release RUNTIME DESTINATION bin)
