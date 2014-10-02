	SECTION "Test",CODE[0]

Start:
	jr		Start
	jr		nz,Start
	jr		z,Start
	jr		nc,Start
	jr		c,Start