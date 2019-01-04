add_executable (motor0x10c
    0x10c_errors.c
    0x10c_errors.h
    0x10c_tokens.c
    0x10c_tokens.h
    0x10c_options.c
    0x10c_options.h
    0x10c_parse.c
    0x10c_symbols.c
    0x10c_main.c)

target_link_libraries (motor0x10c xasm util)
