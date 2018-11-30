add_executable (xlib 
    library.c
    library.h
    libwrap.h
    main.c)

target_link_libraries (xlib util)
