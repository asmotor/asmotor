project(token_generator)

add_executable(generate-tokens
    main.c
)

install(TARGETS generate-tokens)
