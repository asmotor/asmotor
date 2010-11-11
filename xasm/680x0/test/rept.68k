*Comment		

Macro1:	MACRO
		IFND	Undefined
			WARN	"Line 5"
			WARN	"Macro Line 2 \1 (Arg0 = \0)"
			IFND	Undefined2
				WARN	"Line 8"
				WARN	"Macro Line 4 \2"
				PRINTT	{__FILE}
				PRINTT	"{__FILE} {__LINE} {__DATE} {__TIME}\n"
			ENDC
			WARN	"Line 13"
		ELSE
			WARN	"Line 15"
		ENDC
		WARN	"Line 17"
		ENDM
		WARN	"Line 19"
		Macro1.w	<Arg1 2>,3	;Comment
		WARN	"Line 21"
		REPT	2
		WARN	"Line 23"
		WARN	"Outer"	* 2
		REPT	2
		WARN	"Line 26"
		WARN	"Inner"
		IFD		Macro1
			WARN	"Line 29"
		ELSE
			IFND	Undefined2
				WARN	"Line 32"
			ENDC
			WARN	"Line 34"
		ENDC
		WARN	"Line 36"
		ENDR
		WARN	"Line 38"
		REXIT
		ENDR
		WARN	"Line 41"
		IFD		Macro1
			WARN	"Line 43"
		ELSE
			IFND	Undefined2
				WARN	"Line 46"
			ENDC
			WARN	"Line 48"
		ENDC

		WARN	"Line 51"
		
		REPT	2

		ENDR
		
		WARN	"Line 57"
		
		REPT	2
		REXIT
		
		ENDR
		
		WARN	"Line 64"
		
SetTest		SET	1
		PRINTV	SetTest
SetTest = SetTest+1
		PRINTV	SetTest*2

		SECTION	"Code",CODE

		__DCB	0,1,2
		CNOP	0,8
