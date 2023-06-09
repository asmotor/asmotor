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
