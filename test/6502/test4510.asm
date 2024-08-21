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
	DB	$2B
	bit	$1234
	and	$1234
	rol	$1234
	bbr2	$12,@

