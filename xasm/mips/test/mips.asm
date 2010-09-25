	SECTION "Test",CODE[0]

Begin:
	add	r1,r2,r0
	mul	at,t0,t4

	bne	t4,t5,Begin
	bal	Begin
	bgtz	r4,Begin

	sra	t0,t1,31

Label:
	lw	t5,5(t6)

	msub	t5,t6

	seb	s0,s4

	tne	t7,t8
	teq	r0,r1,$10
	tltiu	t5,$50

	clz	r3,r4

	nop

	ei
	di	r4

	mfhi	r5
	jr.hb	r7
	mtlo	r9

	jal	Label
