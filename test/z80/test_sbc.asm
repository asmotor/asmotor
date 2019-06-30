	SECTION	"Test",CODE[0]

	sbc		a,(hl)
	sbc		a,(ix+$7F)
	sbc		a,(iy)
	sbc		a,b
	sbc		a,c
	sbc		a,d
	sbc		a,e
	sbc		a,h
	sbc		a,l
	sbc		a,a
	sbc		a,-1
	sbc		hl,bc
	sbc		hl,de
	sbc		hl,hl
	sbc		hl,sp

	OPT		mu1

	sbc		a,ixl
	sbc		a,iyh
