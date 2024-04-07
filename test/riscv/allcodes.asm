	SECTION "Test",CODE[$100]

    add x14,x3,x2
    add a4,gp,sp
    addi t0,s0,-500
label:
    and x2,x7,x31
    and sp,t2,t6
    andi t0,s0,500
    auipc t0,$2345
    beq t0,s6,label2
    bge t0,s6,label
label2:
    bgeu t0,s6,label
    blt t0,s6,label
    bltu t0,s6,label
    bne t0,s6,label
    fence rw,io
    jal a2,label2