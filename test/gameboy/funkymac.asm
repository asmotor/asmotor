; This example was originally submitted by Harry P. Mulder. I have since
; modified it a bit so I take the blame if anything goes wrong and Harry
; takes the honour if everything's fine! ;)

ldw:	MACRO

; We purge the intermediate PAR1 and PAR2 string symbols to keep the
; symbol table nice and small. My mum always told my that it was good
; programming practice to clean up after myself

PAR1\@	EQUS	STRSUB("\1", 1, 1)
PAR2\@	EQUS	STRSUB("\2", 1, 1)

	ld	PAR1\@,PAR2\@
	PURGE	PAR1\@,PAR2\@

PAR1\@	EQUS	STRSUB("\1", 2, 1)
PAR2\@	EQUS	STRSUB("\2", 2, 1)

	ld	PAR1\@,PAR2\@
	PURGE	PAR1\@,PAR2\@

	ENDM

	SECTION	"FunkyMacroTest",CODE
	ldw	bc,de
