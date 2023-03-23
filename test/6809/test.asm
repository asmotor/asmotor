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

	nop
