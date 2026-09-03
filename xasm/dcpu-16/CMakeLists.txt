add_executable (motordcpu16
    dcpu16_errors.c
    dcpu16_options.c
    dcpu16_parse.c
    dcpu16_section.c
    dcpu16_symbols.c
    dcpu16_tokens.c
    dcpu16_main.c)

target_link_libraries (motordcpu16 motor util)

install (TARGETS motordcpu16 CONFIGURATIONS Release RUNTIME DESTINATION bin)
