	OPT	el

	SECTION	"Test",CODE[0]

Scope:

	RSSET	30
Test1	__RSW	1
Test2	__RSW	1
Test3	__RSL	4
	__RSW	2
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

	PRINTT	"Test_SIZEOF (should be $34): "
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

Struct	RSSET	10
.member	__RSW	1
.SIZEOF	__RSW	0

	PRINTV	Struct\.SIZEOF
	PRINTT	"\n"
	PRINTV	Struct.member
	PRINTT	"\n"
	PRINTT	"Struct = {Struct}\nStruct.SIZEOF = {Struct.SIZEOF}\n"
	PRINTT	"Struct = {Struct}\nStruct.SIZEOF = {Struct\.SIZEOF}\n"

Struct2	RSRESET
.member	__RSW	1
	RSEND

	PRINTV	Struct2
	PRINTT	"\n"
	PRINTV	Struct2.member
	PRINTT	"\n"

