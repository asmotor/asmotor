add_executable (motormips
    mips_errors.c
    mips_errors.h
    mips_main.c
    mips_options.c
    mips_options.h
    mips_parse_instr.c
    mips_parse.c
    mips_symbols.c
    mips_tokens.c
    mips_tokens.h
    )

target_link_libraries (motormips xasm util)
