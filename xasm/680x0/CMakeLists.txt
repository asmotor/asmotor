add_executable (motor68k
    addrmode.h
    fpuinstr.h
    intinstr.h
    localasm.h
    loccpu.h
    locerror.c
    loclexer.c
    locopt.c
    locopt.h
    locparse.c
    locsym.c
    main.c
    pseudoop.h)

target_link_libraries (motor68k xasm util)
