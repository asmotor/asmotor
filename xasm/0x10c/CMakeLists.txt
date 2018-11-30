add_executable (motor0x10c
    localasm.h
    loccpu.h
    locerror.c
    loclexer.c
    loclexer.h
    locopt.c
    locopt.h
    locparse.c
    locsym.c
    main.c)

target_link_libraries (motor0x10c xasm util)
