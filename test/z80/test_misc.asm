	SECTION	"Test",CODE[0]

Start:
	ccf
	cpl
	daa
	di
	djnz	Start
	ei
	halt
	im		0
	im		1
	im		2
	ldd
	lddr
	ldi
	ldir
	neg
	nop
	scf
