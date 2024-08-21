	OPT mc5

	SECTION "Test",CODE[0]

	ora	($12),z
	brk
	ora	($11,x)
	cle
	see
	tsb	$14
	ora	$12
	asl	$12
	rmb0	$12

	php
	ora	#$12
	asl
	tsy
	tsb	$1234
	ora	$1234
	asl	$1234
	bbr0	$12,@

	bpl	@
	ora	($fe),y
	ora	($12),z
	lbpl	@
	trb	$14
	ora	$12,x
	asl	$12,x
	rmb1	$12

	clc
	ora	$1234,y
	ina
	inc	a
	inz
	trb	$1234
	ora	$1234,x
	asl	$1234,x
	bbr1	$12,@

	jsr	@
	and	($12,x)
	jsr	($1234)
	jsr	($1234,x)
	bit	$12
	and	$12
	rol	$12
	rmb	2,$12

	plp
	and	#$12
	rol
	tys
	bit	$1234
	and	$1234
	rol	$1234
	bbr2	$12,@

	bmi	@
	and	($12),y
	and	($12)
	lbmi @
	bit	$12,x
	and	$12,x
	rol	$12,x
	rmb3	$12

	sec
	and	$1234,y
	dec	a
	dea
	dez
	bit	$1234,x
	and	$1234,x
	rol	$1234,x
	bbr3	$12,@

	rti
	eor	($12,x)
	neg
	asr
    asr $1234
	eor	$12
	lsr	$12
	rmb4	$12

	pha
	eor	#$12
	lsr
	taz
	jmp	$1234
	eor	$1234
	lsr	$1234
	bbr	4,$12,@
