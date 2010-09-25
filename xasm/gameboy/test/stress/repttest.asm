
John:	MACRO
	PRINTT	"MACRO: (\@), arg1 \1\n"
	REPT	2
	PRINTT	"MACROREPT * 2 (\@), arg1 \1, narg {__NARG__}\n"
	SHIFT
	ENDR
	PRINTT	"MACROEND: (\@), arg1 \1\n"
	ENDM

	John	3987,1234
	John	3987,1234
	John	3987,1234

	REPT	3
	PRINTT	"1st REPT * 3 (start \@)\n"
		REPT	4
		PRINTT	"\t2nd REPT * 4 (\@)\n"
		ENDR
	PRINTT	"1st REPT * 3 (end \@)\n"
	ENDR

	John	3987,1234
	John	3987,1234
	John	3987,1234

	PRINTT	"Wait...\n"

	REPT	3
	PRINTT	"1st REPT * 3 (start \@)\n"
		REPT	4
		PRINTT	"\t2nd REPT * 4 (\@)\n"
		ENDR
	PRINTT	"1st REPT * 3 (end \@)\n"
	ENDR


	John	3987,1234
	John	3987,1234
	John	3987,1234

	PRINTT	"\nGameboy pixels '33221100' is "
	PRINTV	`33221100
	PRINTT	" in hex!\n\n"