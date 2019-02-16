add_executable (xlib 
    library.c
    library.h
    module.h
    main.c)

target_link_libraries (xlib util)
