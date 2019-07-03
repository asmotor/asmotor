	SECTION	"Test",CODE[0]
Symbol:
	adc		a,(hl)
	adc		a,(ix+$7F)
	adc		a,(iy)
	adc		a,b
	adc		a,c
	adc		a,d
	adc		a,e
	adc		a,h
	adc		a,l
	adc		a,a
	adc		a,-1
	adc		hl,bc
	adc		hl,de
	adc		hl,hl
	adc		hl,sp

	OPT		mu1

	adc		a,ixl
	adc		a,iyh