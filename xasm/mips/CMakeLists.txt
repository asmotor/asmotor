add_executable (motormips
    mips_errors.c
    mips_errors.h
    mips_main.c
    mips_options.c
    mips_options.h
    mips_parse_instr.c
    mips_parse.c
    mips_symbols.c
    mips_symbols.h
    mips_tokens.c
    mips_tokens.h
    )

target_link_libraries (motormips xasm util)

if(NOT MSVC)
    target_link_libraries (motormips m)
endif(NOT MSVC)

install (TARGETS motormips CONFIGURATIONS Release RUNTIME DESTINATION bin)
