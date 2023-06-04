	RSSET	8
t0	RB	1
t1	RB	1
t2	RB	1
t3	RB	1
t4	RB	1
t5	RB	1
t6	RB	1
t7	RB	1

	opt	mc2

	SECTION	"Test",CODE[$0]
Start:
		lda	$02,y

		lda	(t5)
		cmp	#4+1; SPRITES_PER_BUCKET*2+1;
		beq	.skip

		tay
		pla
		sta	(t5),y
		iny
		pla
		sta	(t5),y
		iny
		tya
		lda	#3
		sta	(t5)

.skip


	adc	#$44
	adc	$44
	adc	$44,x
	adc	$4400
	adc	$4400,x
	adc	$4400,y
	adc	($44,x)
	adc	($44),y
	asl	a

	bit	$44

	beq	Loop
	bne	Start
	
Loop:
	brk
	cmp	$1234
	cpx	#$12
	cpy	$1234
	dec	$1234,x
	eor	($12),y

	clc
	sec
	cli
	sei
	clv
	cld
	sed

	inc	$12,x
	inc	<$23,x
	
	jmp	Loop
	jmp	($1234)
	jsr	Loop

	lda	#>$1234
	lda	Loop
	ldx	Loop,Y
	ldx	#$12
	ldy	#$12

	lsr	a
	nop
	ora	$12

	tax
	txa
	dex
	inx
	tay
	tya
	dey
	iny

	rol	a
	ror	a
	rti
	rts
