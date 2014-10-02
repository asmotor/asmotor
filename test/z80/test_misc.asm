	SECTION	"Test",CODE[0]

Start:
	ccf
	cpl
	daa
	di
	djnz	Start
	ei