; macro.asm
	SECTION "Code",CODE

Function:	MACRO	;funcname
	XDEF	\1
\1:
	ENDM

	Function	Start

Second:	MACRO
	PRINTT	"No ending"
	