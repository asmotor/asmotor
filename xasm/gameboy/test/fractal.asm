	SECTION	"Mandelbrot",CODE

PlotChar:	MACRO
	PRINTT	strsub(".'-=+#*!?%&$?œ@ ",(\1&$F)+1,1)
	ENDM

Iterate:	MACRO
	IF	(MUL(_x,_x)+MUL(_y,_y)<4.0) && (_iterations<63)
_newx	SET	MUL(_x,_x)-MUL(_y,_y)+_cx
_newy	SET	MUL(_x,_y)*2+_cy
_iterations	SET	_iterations+1
_x	SET	_newx
_y	SET	_newy
	Iterate
	ELSE
	PlotChar _iterations
	ENDC
	ENDM

DrawChar:	MACRO
	IF	_cx<1.0
_x	SET	0
_y	SET	0
_iterations	SET	0
	Iterate
_cx	SET	_cx+_cx_inc
	DrawChar
	ENDC
	ENDM

DrawLine:	MACRO
	IF	_cy<1.0
_cx	SET	-2.0
_cx_inc	SET	DIV(3.0,70.0)
	DrawChar
_cy	SET	_cy+_cy_inc
	PRINTT	"\n"
	DrawLine
	ELSE
	PRINTT	"Done!\n"
	ENDC
	ENDM

_cy	SET	-1.0
_cy_inc	SET	DIV(2.0,40.0)
	DrawLine