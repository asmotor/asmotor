	REPT	2
	REXIT
	ENDR
	WARN	"Line 4"

Test:	MACRO
	PRINTT	"MACRO: (\@), arg1 \1\n"
	REPT	2
	PRINTT	"MACROREPT * 2 (\@), arg1 \1, narg {__NARG}\n"
	SHIFT
	ENDR
	PRINTT	"MACROEND: (\@), arg1 \1\n"
	ENDM

	REPT	2
	Test	3987,1234
	Test	3987,1234
	Test	3987,1234

	REPT	3
	PRINTT	"1st REPT * 3 (start \@)\n"
		REPT	4
		PRINTT	"\t2nd REPT * 4 (\@)\n"
		ENDR
	PRINTT	"1st REPT * 3 (end \@)\n"
	ENDR
	
	ENDR

	WARN	"Line 30"

	Test	3987,1234
	Test	3987,1234
	Test	3987,1234

