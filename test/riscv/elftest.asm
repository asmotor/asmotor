    IMPORt extsymbol

    SECTION "Test",CODE
symbol:
    IF 0
    jal symbol2
    jal extsymbol+4
    j extsymbol
    jal extsymbol
    beq t0,t1,symbol
    beq t0,t1,symbol2-4
    beq t0,t1,symbol2
    ENDC
    lw x1,extsymbol
symbol2:
