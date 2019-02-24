	OPT	el

	SECTION	"Test",CODE[0]

Scope:

	RSSET	30
Test1	__RSW	1
Test2	__RSW	1
Test3	__RSL	4
Test_SIZEOF	__RSW	0

	PRINTT	"Test1 (should be $1E): "
	PRINTV	Test1
	PRINTT	"\n"

	PRINTT	"Test2 (should be $20): "
	PRINTV	Test2
	PRINTT	"\n"

	PRINTT	"Test3 (should be $22): "
	PRINTV	Test3
	PRINTT	"\n"

	PRINTT	"Test_SIZEOF (should be $32): "
	PRINTV	Test_SIZEOF
	PRINTT	"\n"

TestMacro:	MACRO
	PRINTT	"\@"
	ENDM

	TestMacro
	TestMacro
	TestMacro
	TestMacro
	TestMacro
	TestMacro
	PRINTT	"\n"
