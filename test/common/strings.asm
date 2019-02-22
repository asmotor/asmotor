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

	PRINTT	"\n\"bcdefgh\" > \"abcdefg\": "
	PRINTV	"bcdefgh">"abcdefg"

	PRINTT	"\n\"abcdefgh\" == \"abcdefg\": "
	PRINTV	"abcdefgh"=="abcdefg"

	PRINTT	"\n\"abcdefgh\" == \"abcdefgh\": "
	PRINTV	"abcdefgh"=="abcdefgh"
	
	PRINTT	"\n\"1234\": "
	PRINTV	"1234"

Test	EQUS	"This is a test"
	PRINTT	"\n{|Test|}"
	PRINTT	|Test|.slice(|Test|.indexof(" is"),20)
	PRINTT	"\n"
	PRINTV	|Test|.slice(|Test|.indexof(" is"),20).upper.length
	PRINTT	"\n"

	PRINTT	"0123".slice(4,1)+"No character before this text\n"

	PRINTT	"|Test|.length == "
	PRINTV	|Test|.length
	PRINTT	"\n"

	PRINTT	|Test|+" of string concatenation\n"

	PRINTT	"This should be upper case\n".upper
	PRINTT	"This should be lower case\n".lower

	PRINTT	"This should be {"upper".upper} case\n"

	PRINTT	"{-16,6:D3}\n"
	PRINTT	"{65:C}\n"
	PRINTT	"{87}\n"
	PRINTT	"{20:X4}\n"
	PRINTT	"{15.6,9:F2}\n"
	PRINTT	"{15.6,-9:F2}End\n"
	PRINTT	"{-1234:X6}\n"
	PRINTT	"{|Test|,30}\n"
	PRINTT	"{"Text",9}\n"
