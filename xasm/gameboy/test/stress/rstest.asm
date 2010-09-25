	SECTION	"John",HOME

Scope:

	RSSET	30
John1	RB	1
John2	RW	1
John3	RW	4
John_SIZEOF	RB	0

	PRINTT	"John1 (should be $1E): "
	PRINTV	John1
	PRINTT	"\n"

	PRINTT	"John2 (should be $1F): "
	PRINTV	John2
	PRINTT	"\n"

	PRINTT	"John3 (should be $21): "
	PRINTV	John3
	PRINTT	"\n"

	PRINTT	"John_SIZEOF (should be $29): "
	PRINTV	John_SIZEOF
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

	INCBIN	"rstest.z80"

