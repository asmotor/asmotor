add_executable (motorv
    v_errors.c
    v_main.c
    v_options.c
    v_parse.c
    v_section.c
    v_symbols.c
    v_tokens.c
    )

target_link_libraries (motorv motor util)

if(NOT MSVC)
    target_link_libraries (motorv m)
endif(NOT MSVC)

install (TARGETS motorv CONFIGURATIONS Release RUNTIME DESTINATION bin)
