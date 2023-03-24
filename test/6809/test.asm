	SECTION	"Test",CODE[$0]

Start:
	abx
	adca	#12
	adca	<$1234
	adca	$12
	adca	$1234
	adca	[$1234]
	adca	>$12
	adca	[>$12]
	adca	2,y
	adca	,x+
	adca	,s++
	adca	[,s++]
	adca	,-u
	adca	,--u
	adca	[,--u]
	adca	,s
	adca	[,s]
	adca	a,s
	adca	[a,s]
	adca	b,x
	adca	[b,x]
	adca	$40,s
	adca	[$40,s]
	adca	$1234,s
	adca	[$1234,s]
	adca	d,x
	adca	[d,x]
	adca	[$1234,pcr]
	adca	[<$34,pcr]

	adcb	,u
	adda	#87
	addb	4,s
	addd	#$1234
	anda	#$01
	andb	$3987
	andcc	#$42

	asla
	aslb
	asl	$1234
	lsl	[1,s]

.loop	asra
	asrb
	asr	$1234

	bra	.loop
	lbcc	.loop
	lbra	.loop
	bsr	.loop
	lbsr	.loop

	bita	3,y
	bitb	#2

	clr	6,s
	clra
	clrb

	cmpa	#4
	cmpb	,s+
	cmpd	#4
	cmpx	4
	cmpy	a,x
	cmpu	,y
	cmps	,s

	com	[,y]
	coma
	comb
	cwai	#15
	daa
	dec	[5,u]
	deca
	decb
	eora	,s
	eorb	<$45
	exg	a,b
	exg	d,u
	inc	[5,u]
	inca
	incb
	jmp	$1234
	jmp	<$1234
	jmp	[$1234]
	jsr	$1234
	jsr	<$1234
	jsr	[$1234]

	lda	#1
	ldb	256
	ldd	#25
	ldx	5,s
	ldy	[,u]
	ldu	,y
	lds	[a,s]

	leax	,y
	leay	,s
	leau	a,u
	leas	[a,u]
	lsr	4,s
	mul
	neg	[,y++]
	nega
	negb
	ora	#$01
	orb	$3987
	orcc	#$7

	pshs	a
	pshu	b,x,s
	puls	b,y,u
	pulu	b

	rol	5
	rola
	rolb
	ror	5,x
	rora
	rorb

	rti
	rts

	sbca	#12
	sbcb	12,y

	sta	,x
	stb	,y
	std	25
	std	256
	std	[1]
	stx	5,s
	sty	[,u]
	stu	,y
	sts	[a,s]

	nop
