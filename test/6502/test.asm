	SECTION	"Test",CODE[$0]
Start:
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
