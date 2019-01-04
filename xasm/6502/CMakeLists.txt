add_executable (motor6502
    intinstr.h
    localasm.h
    loccpu.h
    locerror.c
    loclexer.c
        tokens.h
    locopt.c
    locopt.h
    locparse.c
    locsym.c
    main.c)

target_link_libraries (motor6502 xasm util)
