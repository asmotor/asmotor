add_executable (motorschip 
    intinstr.h
    localasm.h
    loccpu.h
    locerror.c
    loclexer.c
    locopt.c
    locopt.h
    locparse.c
    locsym.c
    main.c)

target_link_libraries (motorschip xasm util)
