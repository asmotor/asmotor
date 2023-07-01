add_executable (xlink
    amiga.c
    assign.c
    coco.c
    commodore.c
	elf.c
	error.c
	foenix.c
    gameboy.c
    group.c
    hc800.c
    image.c
    main.c
    mapfile.c
    memorymap.c
    object.c
    patch.c
    section.c
    sega.c
    smart.c)

target_link_libraries (xlink util)

if(NOT MSVC)
    target_link_libraries (xlink m)
endif(NOT MSVC)

install (TARGETS xlink CONFIGURATIONS Release RUNTIME DESTINATION bin)
