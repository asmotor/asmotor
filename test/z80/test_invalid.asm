	SECTION	"Invalid opcodes",CODE[0]

Data:	EQU	$1234

	ld		(Data),af
	stop
	ld		(hl+),a
	ldi		(hl),a
	ld		(hl-),a
	ldi		(hl),a
	out		($ff00+c),a
	ldh		($ff),a
	add		sp,1
	ldhl	sp,2