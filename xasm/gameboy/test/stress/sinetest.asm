
_count	SET	0

	REPT	256
	PRINTT	"sin({_count}) == "
	PRINTV	sin(_count)
	PRINTT	" or "
	PRINTF	sin(_count)
	PRINTT	"\n"
_count	SET	_count+(65536/256)<<16
	ENDR

	PRINTT	"PI equals {_PI} or "
	PRINTF	_PI
	PRINTT	"\n"
