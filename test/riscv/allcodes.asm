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
    jalr s3,label2(s0)
    lb t0,label(sp)
    lbu s1,87(s0)
    lh t0,label(sp)
    lhu s1,87(s0)
    lhu s1,(s0)
    lui t0,12345
    lw t0,-42(t1)
    or t0,t1,t2
    ori t0,t1,123
    sb t0,87(t1)
    sb t0,87(t1)
    sh t0,(t1)
    sh t0,87(t1)
    sll t0,t1,t2
    slli t0,t1,5
    slt t0,t1,t2
    slti t0,t1,-12
    sltiu t0,t1,-1500
    sltiu t0,t1,$FFFFFF80
    sltu t0,t1,t2
    sra t0,t1,t2
    srai t0,t1,5
    srl t0,t1,t2
    srli t0,t1,5
    sub x14,x3,x2
    sw t0,87(t1)
    sw t0,(t1)
    xor x2,x7,x31
    xor sp,t2,t6

    ; pseudoinstructions

    j label2
    jal label2
    jr s1
    jalr s1
    ret

    beqz s0,label2
    bnez s0,label2
    blez s0,label2
    bgez s0,label2
    bltz s0,label2
    bgtz s0,label2
    bgt t0,s6,label
    ble t0,s6,label
    bgtu t0,s6,label
    bleu t0,s6,label

    mv t0,t1
    neg t0,t1
    not t0,t1
    nop

    seqz s0,s1
    snez s0,s1
    sltz s0,s1
    sgtz s0,s1

    lhu s1,@+8
    lw t0,label2
    sb s1,label2,t0
    sw s1,label2,t0

    li s0,87
    li s0,-87
    li s0,$1000
    li s0,$FFFF1000
    li s0,$12345678
    li s0,$12345FED
    la t0,label2
    lla t0,label2

    call label2
    tail label2

    fence

    ebreak
    ecall
    
    OPT mpriv

    mret
    sret
    wfi

