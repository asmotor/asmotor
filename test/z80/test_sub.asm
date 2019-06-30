	SECTION	"Test",CODE[0]

	sub		(hl)
	sub		a,(ix+2)
	sub		a,(iy-2)
	sub		a,b
	sub		a,c
	sub		a,d
	sub		a,e
	sub		a,h
	sub		a,l
	sub		a,a
	sub		a,2

	OPT		mu1

	sub		a,ixl
	sub		a,iyh
