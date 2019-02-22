	IF	2+2==5
		PRINTT	"Expect $3:"
		PRINTV	__LINE
		PRINTT	"\n"
		PRINTT	"2+2==5\n"
	ELSE
		PRINTT	"Expect $8:"
		PRINTV	__LINE
		PRINTT	"\n"
		PRINTT	"2+2!=5\n"
		IF	2+2~=4
			PRINTT	"Expect $D:"
			PRINTV	__LINE
			PRINTT	"\n"
			PRINTT	"2+2~=4\n"
		ELSE
			PRINTT	"Expect $12:"
			PRINTV	__LINE
			PRINTT	"\n"
			PRINTT	"2+2==4\n"
		ENDC
		PRINTT	"Expect $17:"
		PRINTV	__LINE
		PRINTT	"\n"
	ENDC

	PRINTT	"Expect $1C:"
	PRINTV	__LINE
	PRINTT	"\n"
