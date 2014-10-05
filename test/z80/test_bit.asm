	SECTION	"Test",CODE[0]

	bit		0,(hl)
	bit		7,(hl)
	bit		1,(ix+2)
	bit		2,(iy-3)
	bit		0,b
	bit		0,c
	bit		0,d
	bit		0,e
	bit		0,h
	bit		0,l
	bit		0,a

	res		0,(hl)
	res		7,(hl)
	res		1,(ix+2)
	res		2,(iy-3)
	res		0,b
	res		0,c
	res		0,d
	res		0,e
	res		0,h
	res		0,l
	res		0,a

	set		0,(hl)
	set		7,(hl)
	set		1,(ix+2)
	set		2,(iy-3)
	set		0,b
	set		0,c
	set		0,d
	set		0,e
	set		0,h
	set		0,l
	set		0,a
