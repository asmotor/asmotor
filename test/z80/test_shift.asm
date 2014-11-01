	SECTION	"Test",CODE[0]

	sla		(hl)
	sla		(ix+1)
	sla		(iy+2)
	sla		b
	sll		(hl)
	sll		(ix+1)
	sll		(iy+2)
	sll		c
	sra		(hl)
	sra		(ix+1)
	sra		(iy+2)
	sra		d
	srl		(hl)
	srl		(ix+1)
	srl		(iy+2)
	srl		e
