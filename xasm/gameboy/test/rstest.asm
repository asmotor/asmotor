	SECTION	"John",CODE

Scope:

	RSSET	30
Test1	RB	1
Test2	RW	1
Test3	RW	4
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
.label\@	DW	.label\@
	ENDM

	TestMacro
	TestMacro
	TestMacro
	TestMacro
	TestMacro
	TestMacro
	PRINTT	"\n"

	INCBIN	"rstest.asm"

