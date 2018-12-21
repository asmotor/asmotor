	PRINTT	"\"long\"'s position in \"A reasonably long string\": "
	PRINTV	"A reasonably long string".indexof("long")

	PRINTT	"\n\"two\"'s position in \"Another one\": "
	PRINTV	"Another one".indexof("two")

	PRINTT	"\n\"abc\" compared to \"abc\": "
	PRINTV	"abc".compareto("abc")

	PRINTT	"\n\"abc\" compared to \"bcd\": "
	PRINTV	"abc".compareto("bcd")

	PRINTT	"\n\"bcd\" compared to \"abc\": "
	PRINTV	"bcd".compareto("abc")

	PRINTT	"\n\"bcd\" > \"abc\": "
	PRINTV	"bcd">"abc"

	PRINTT	"\n\"abc\" == \"abcd\": "
	PRINTV	"abc"=="abcd"

	PRINTT	"\n\"abc\" == \"abc\": "
	PRINTV	"abc"=="abc"
	
	PRINTT	"\n\"1234\": "
	PRINTV	"1234"

Test	EQUS	"This is a test"
	PRINTT	"\n{Test}"
	PRINTT	{Test}.slice({Test}.indexof(" is"),20)
	PRINTT	"\n"
	PRINTV	{Test}.slice({Test}.indexof(" is"),20).toupper().length
	PRINTT	"\n"

	PRINTT	"0123".slice(4,1)+"No character before this text\n"

	PRINTT	"\{Test\}\.length == "
	PRINTV	{Test}.length
	PRINTT	"\n"

	PRINTT	{Test}+" of string concatenation\n"

	PRINTT	"This should be upper case\n".toupper()
	PRINTT	"This should be lower case\n".tolower()

	