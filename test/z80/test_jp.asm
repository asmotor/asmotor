	SECTION	"Test",CODE[0]

	jp		$1234
	jp		(hl)
	jp		(ix)
	jp		(iy)
	jp		nz,$1000
	jp		z,$1000
	jp		nc,$1000
	jp		c,$1000
	jp		po,$1000
	jp		pe,$1000
	jp		p,$1000
	jp		m,$1000
