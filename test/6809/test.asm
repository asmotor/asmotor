	SECTION	"Test",CODE[$0]

Start:
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

	abx
	adca	,x
	adca	4,y
	adcb	,u
	adcb	[,x]
	nop
	