add_executable (motordcpu16
    dcpu16_errors.c
    dcpu16_errors.h
    dcpu16_options.c
    dcpu16_options.h
    dcpu16_parse.c
    dcpu16_parse.h
    dcpu16_symbols.c
    dcpu16_symbols.h
    dcpu16_tokens.c
    dcpu16_tokens.h
    dcpu16_main.c)

target_link_libraries (motordcpu16 xasm util)

if(NOT MSVC)
    target_link_libraries (motordcpu16 m)
endif(NOT MSVC)

install (TARGETS motordcpu16 CONFIGURATIONS Release RUNTIME DESTINATION bin)
