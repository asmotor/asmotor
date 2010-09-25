
	SECTION	"Test",CODE

John	EQUS	"\tPRINTT \"Weee\\n\"\nPRINTT \"Dang\\n\"\n"
John2	EQUS	"[hl]"

	REPT	3
	John
	REPT	2
	PRINTT	"\t2nd REPT\n"
	ENDR
	ENDR

	nop