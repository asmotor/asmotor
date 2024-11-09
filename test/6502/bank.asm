	SECTION	"Code",CODE[0],BANK[42]

Test:
	DB	BANK(Test)


M:	MACRO
	DB	BANK(\1)
	ENDM

	M Test
	