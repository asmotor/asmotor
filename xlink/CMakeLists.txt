add_executable (xlink
    amiga.c
    amiga.h
    assign.c
    assign.h
    commodore.c
    commodore.h
    gameboy.c
    gameboy.h
    group.c
    group.h
    image.c
    image.h
    main.c
    mapfile.c
    mapfile.h
    object.c
    object.h
    patch.c
    patch.h
    section.c
    section.h
    sega.c
    sega.h
    smart.c
    smart.h
    symbol.h
    xlink.h)

target_link_libraries (xlink util)
install (TARGETS xlink CONFIGURATIONS Release RUNTIME DESTINATION bin)
