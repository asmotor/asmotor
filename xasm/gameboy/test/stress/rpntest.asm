	SECTION	"John",HOME

	; NOTE!
	; Because the ampersand (&) is used both for the octal prefix
	; and for bitwise and, you can't and with a decimal value.
	; There are two possible workarounds. Convert the number to
	; a different base r put it in brackets ()

	ld	de,((1+2)*3/4|(5<<6))&$7>8>>9
