	OPT	mc3
	BITS	16,16

	SECTION	"Test",CODE
	adc	12,S
	adc	[12]
	adc	#$1234
	adc	$123456
	adc	(12,S),Y
	adc	[$12],Y
	adc	>$1234,X

	sbc	12,S
	sbc	[12]
	sbc	#$1234
	sbc	$123456
	sbc	(12,S),Y
	sbc	[$12],Y
	sbc	>$1234,X

	cmp	12,S
	cmp	[12]
	cmp	#$1234
	cmp	$123456
	cmp	(12,S),Y
	cmp	[$12],Y
	cmp	>$1234,X

	cpx	#$12
	cpx	$12
	cpx	$1234
	
	cpy	#$12
	cpy	$12
	cpy	$1234

	and	12,S
	and	[12]
	and	#$1234
	and	$123456
	and	(12,S),Y
	and	[$12],Y
	and	>$1234,X

	eor	12,S
	eor	[12]
	eor	#$1234
	eor	$123456
	eor	(12,S),Y
	eor	[$12],Y
	eor	>$1234,X

	ora	12,S
	ora	[12]
	ora	#$1234
	ora	$123456
	ora	(12,S),Y
	ora	[$12],Y
	ora	>$1234,X

	bit	#$1234

	jml	$123456
	jmp	>$123456
	jmp	[$1234]
	jsl	$123456
	jsr	>$123456
	jsr	($1234,X)

	rtl

	rep	#$12
	sep	#$12

	lda	12,S
	lda	[12]
	lda	#$1234
	lda	$123456
	lda	(12,S),Y
	lda	[$12],Y
	lda	$123456,X
	ldx	#$1234
	ldy	#$1234

	sta	12,S
	sta	[12]
	sta	$123456
	sta	(12,S),Y
	sta	[$12],Y
	sta	$123456,X

	mvn	#$12,#$34
	mvp	#$12,#$34
	
label	pea	$1234
	pei	(12)
	per	label

	phb
	phd
	phk
	php
	plb
	pld
	plp

	txy
	tyx

	tcd
	tcs
	tdc
	tsc

	xba
	xce

	lda	#^$123456
