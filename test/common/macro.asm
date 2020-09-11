; macro.asm
	SECTION "Code",CODE

WaitBlt:	MACRO
.vent\@:	if	Raster=1
	move.w	#$fff,Color00+_Custom
	endif
	btst	#14,Dmaconr+_Custom
	bne.b	.vent\@
	if	Raster=1
	move.w	#0,Color00+_Custom
	endif
	ENDM

Function:	MACRO	;funcname
	XDEF	\1
\1:
	ENDM

	Function	Start
