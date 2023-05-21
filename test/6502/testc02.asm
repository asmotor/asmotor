	OPT	mc2

	SECTION	"Test",CODE[$0]
Start:
	adc	$12
	tsb	$87
	rmb0	$12
	smb	1,$12
	tsb	$1234
.loop	bbr5	$12,.loop

