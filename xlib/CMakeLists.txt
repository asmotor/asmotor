add_executable (xlib 
    library.c
    library.h
    module.h
    main.c)

target_link_libraries (xlib util)
install (TARGETS xlib CONFIGURATIONS Release RUNTIME DESTINATION bin)
