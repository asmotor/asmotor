
	SECTION	"Test",CODE

__equs	EQUS	"label"
__equs#1	EQU	42

	PRINTV	label1


Print	EQUS	"\tPRINTT \"First\\n\"\n\tPRINTT \"Second\\n\"\n"

	REPT	3
	Print
	REPT	2
	PRINTT	"\t2nd REPT\n"
	ENDR
	ENDR

	REPT	3
