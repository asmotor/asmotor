    IMPORt extsymbol

    SECTION "Test",CODE
symbol:
    jal symbol2
    jal extsymbol+4
    j extsymbol
    beq t0,t1,symbol
    beq t0,t1,symbol2-4
    beq t0,t1,symbol2
symbol2:
