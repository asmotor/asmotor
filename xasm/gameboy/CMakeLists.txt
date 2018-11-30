add_executable (motorgb 
    localasm.h
    locerror.c
    loclexer.c
    locopt.c
    locopt.h
    locparse.c
    locsym.c
    main.c)

target_link_libraries (motorgb xasm util)
