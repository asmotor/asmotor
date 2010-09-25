
	IF	!DEF(UTILITY_INC)
UTILITY_INC	SET	1

; --
; -- Externally declared symbols
; --

	GLOBAL	mem_Set
	GLOBAL	mem_Copy
	GLOBAL	fade_Init
	GLOBAL	fade_Decrease
	GLOBAL	fade_Increase
	GLOBAL	fade_Process
	GLOBAL	lcd_DisplayOff
	GLOBAL	lcd_WaitVRAM
	GLOBAL	gbi_Show
	GLOBAL	gbi_VBlank
	GLOBAL	z80_SetBank
	GLOBAL	z80_LongCall
	GLOBAL	z80_LongJp
	GLOBAL	pad_Read

	GLOBAL	_PadData
	GLOBAL	_PadDataEdge

; --
; -- A few macros to do easy far branching
; --

lcall:	MACRO
	ld	a,BANK(\1)
	ld	hl,\1
	call	z80_LongCall
	ENDM

ljp:	MACRO
	ld	a,BANK(\1)
	ld	hl,\1
	jp	z80_LongJp
	ENDM

; --
; -- Other useful macros
; --

pusha:	MACRO
	push	af
	push	bc
	push	de
	push	hl
	ENDM

popa:	MACRO
	pop	hl
	pop	de
	pop	bc
	pop	af
	ENDM

; --
; -- Joypad EQUates
; --

PADF_D	EQU	$80
PADF_U	EQU	$40
PADF_L	EQU	$20
PADF_R	EQU	$10
PADF_START	EQU	$08
PADF_SELECT	EQU	$04
PADF_B	EQU	$02
PADF_A	EQU	$01

; --
; -- STRUCT sFade
; --

	RSRESET
fd_ubIntensity	RB	1
fd_ubPalette	RB	1
fd_ubFade0	RB	1
fd_ubFade1	RB	1
fd_ubFade2	RB	1
fd_ubFade3	RB	1
fd_SIZEOF	RB	0

	ENDC	;UTILITY_INC