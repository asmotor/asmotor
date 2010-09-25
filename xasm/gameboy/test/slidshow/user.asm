;***************************************************************************
;*
;* USER.ASM - User code goes here
;*
;* This is just a slideshow using some of the routines in my tiny library/OS
;*
;***************************************************************************

	INCLUDE	"irq.inc"
	INCLUDE	"utility.inc"
	INCLUDE	"hardware.inc"

	IMPORT	Picture1,Picture2,Picture3,Picture4
	IMPORT	Picture5,Picture6,Picture7,Picture8
	IMPORT	Picture9,Picture10,Picture11,Picture12
	IMPORT	Picture13,Picture14,Picture15;,Picture16

	SECTION	"User",CODE




UserMain::
	; set up my VBlank interrupt

	xor	a,a
	ld	b,BANK(MyVBInt)
	ld	de,MyVBInt
	call	irq_Set

	; fade down the Nintendo logo (hey it looks cool. no, really)

	call	FadeNintendo

	; let's have a slideshow shall we?

.loop	lcall	Picture1
	call	TEST
	lcall	Picture2
	call	TEST
	lcall	Picture3
	call	TEST
	lcall	Picture4
	call	TEST
	lcall	Picture5
	call	TEST
	lcall	Picture6
	call	TEST
	lcall	Picture7
	call	TEST
	lcall	Picture8
	call	TEST
	lcall	Picture9
	call	TEST
	lcall	Picture10
	call	TEST
	lcall	Picture11
	call	TEST
	lcall	Picture12
	call	TEST
	lcall	Picture13
	call	TEST
	lcall	Picture14
	call	TEST
	lcall	Picture15
	call	TEST
;	lcall	Picture16
;	call	TEST
	jp	.loop

; --
; -- TEST logo fader
; --

TEST:	ld	hl,FadeStruct
	xor	a,a
	ld	[hl+],a
	ld	a,%11100100	;our palette
	ld	[hl+],a
	call	fade_Init
	xor	a,a
	call	gbi_VBlank
.fade	call	WaitVB
	ld	hl,FadeStruct
	call	fade_Process
	call	gbi_VBlank
	ld	hl,FadeStruct
	call	fade_Increase
	cp	a,$40
	jr	nz,.fade

.waitbutton	call	WaitVB
	ld	a,%11100100	;our palette
	call	gbi_VBlank
	call	pad_Read
	ld	a,[_PadDataEdge]
	and	a,PADF_A
	cp	a,0
	jr	z,.waitbutton

.fade_down	call	WaitVB
	ld	hl,FadeStruct
	call	fade_Process
	call	gbi_VBlank
	ld	hl,FadeStruct
	call	fade_Decrease
	cp	a,$0
	jr	nz,.fade_down
	ret




; --
; -- Nintendo logo fader
; --

FadeNintendo:	ld	hl,FadeStruct
	ld	a,$40
	ld	[hl+],a
	ld	a,%11111100
	ld	[hl+],a
	call	fade_Init

.fade	call	WaitVB
	ld	hl,FadeStruct
	call	fade_Process
	ld	[_HW+rBGP],a
	ld	hl,FadeStruct
	xor	a,a
	cp	a,[hl]
	ret	z
	call	fade_Decrease
	jr	.fade
	ret

; --
; -- VBlank stuff
; --

WaitVB:	xor	a,a
	ldio	[oVBlank],a
.wait	ldio	a,[oVBlank]
	cp	a,0
	jr	z,.wait
	ret

MyVBInt:	ld	hl,oVBlank
	inc	[hl]
	ret




; --
; -- Variables
; --

	SECTION	"UserVars",HRAM

oVBlank:	DS	1
FadeStruct:	DS	fd_SIZEOF