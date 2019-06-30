	SECTION	"Test",CODE[0]

	cp		(hl)
	cp		(ix+1)
	cp		(iy+2)
	cp		a,b
	cp		a,c
	cp		a,d
	cp		a,e
	cp		a,h
	cp		a,l
	cp		a,a
	cp		a,2

	cpd
	cpdr
	cpi
	cpir

	OPT		mu1
	
	cp		a,ixl
	cp		a,iyh
