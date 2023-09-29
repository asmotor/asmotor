add_executable (motormips
    mips_errors.c
    mips_main.c
    mips_options.c
    mips_parse_instr.c
    mips_parse.c
    mips_section.c
    mips_symbols.c
    mips_tokens.c
    )

target_link_libraries (motormips motor util)
target_include_directories (motormips PRIVATE "${PROJECT_SOURCE_DIR}/xasm/motor")

if(NOT MSVC)
    target_link_libraries (motormips m)
endif(NOT MSVC)

install (TARGETS motormips CONFIGURATIONS Release RUNTIME DESTINATION bin)
