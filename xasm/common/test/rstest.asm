	SECTION	"John",CODE

Scope:

	RSSET	30
Test1	__RSB	1
Test2	__RSW	1
Test3	__RSW	4
Test_SIZEOF	RB	0

	PRINTT	"Test1 (should be $1E): "
	PRINTV	Test1
	PRINTT	"\n"

	PRINTT	"Test2 (should be $1F): "
	PRINTV	Test2
	PRINTT	"\n"

	PRINTT	"Test3 (should be $21): "
	PRINTV	Test3
	PRINTT	"\n"

	PRINTT	"Test_SIZEOF (should be $29): "
	PRINTV	Test_SIZEOF
	PRINTT	"\n"

TestMacro:	MACRO
	PRINTT	"\@"
.label\@	__DCW	.label\@
	ENDM

	TestMacro
	TestMacro
	TestMacro
	TestMacro
	TestMacro
	TestMacro
	PRINTT	"\n"

	INCBIN	"rstest.asm"

