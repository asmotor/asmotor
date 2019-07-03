TestMacro:	MACRO
	ld		a,\1
	ENDM

	SECTION	"Test",CODE[0]

	ex		(sp),hl
	ex		hl,(sp)
	ex		(sp),ix
	ex		ix,(sp)
	ex		(sp),iy
	ex		iy,(sp)
	ex		af,af'
	TestMacro b ; Test
	ex		af',af
	ex		de,hl
	ex		hl,de
	exx
