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

if(NOT MSVC)
    target_link_libraries (motorz80 m)
endif(NOT MSVC)

install (TARGETS motorz80 CONFIGURATIONS Release RUNTIME DESTINATION bin)
