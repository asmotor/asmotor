add_executable (motormips
    intinstr.h
    localasm.h
    loccpu.h
    locerror.c
    locerror.h
    loclexer.c
    locopt.c
    locopt.h
    locparse.c
    locsym.c
    main.c)

target_link_libraries (motormips xasm util)
