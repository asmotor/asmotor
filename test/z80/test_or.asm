	SECTION "Test",CODE[0]

	or (hl)
	or a,(hl)
	or (ix+1)
	or a,(ix+2)
	or (iy+3)
	or a,(iy+4)
	or a,l
	or $42