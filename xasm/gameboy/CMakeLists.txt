add_executable(motorgb
        z80errors.c
        z80errors.h
        z80main.c
        z80options.c
        z80options.h
        z80parse.c
        z80symbols.c
        z80tokens.c
        z80tokens.h
        )

target_link_libraries(motorgb xasm util)
