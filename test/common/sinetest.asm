
_count	SET	0

	REPT	256
_angle	SET	_count*256
	PRINTT	"sin({_angle}) == "
	PRINTV	sin(_angle)
	PRINTT	" or "
	PRINTF	sin(_angle)
	PRINTT	"\n"
_count	SET	_count+1
	ENDR
