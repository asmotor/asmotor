	PRINTT	"\"long\"'s position in \"A reasonably long string\": "
	PRINTV	strin("A reasonably long string","long")

	PRINTT	"\n\"two\"'s position in \"Another one\": "
	PRINTV	strin("Another one","two")

	PRINTT	"\n\"abc\" compared to \"abc\": "
	PRINTV	strcmp("abc","abc")

	PRINTT	"\n\"abc\" compared to \"bcd\": "
	PRINTV	strcmp("abc","bcd")

	PRINTT	"\n\"bcd\" compared to \"abc\": "
	PRINTV	strcmp("bcd","abc")

Test	EQUS	"This is a test"
	PRINTT	"\n{Test}"
	PRINTT	{Test}.slice(strin({Test}," is"),20)
	PRINTT	"\n"

	PRINTT	"strlen(\"{Test}\") == "
	PRINTV	strlen({Test})
	PRINTT	"\n"

	PRINTT	{Test}+" you bastard\n"

	PRINTT	"JohnD\n".toupper()
	PRINTT	"JohnD\n".tolower()

	