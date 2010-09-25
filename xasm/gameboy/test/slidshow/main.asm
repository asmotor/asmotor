;***************************************************************************
;*
;* MAIN.ASM - Standard ROM-image header
;*
;* All fields left to zero since RGBFix does a nice job of filling them in
;* in for us...
;*
;***************************************************************************

	IMPORT	UserMain
	INCLUDE	"irq.inc"
	INCLUDE	"utility.inc"
	INCLUDE	"hardware.inc"

	SECTION	"Startup",HOME[0]

RST_00:	jp	Main
	DS	5
RST_08:	jp	Main
	DS	5
RST_10:	jp	Main
	DS	5
RST_18:	jp	Main
	DS	5
RST_20:	jp	Main
	DS	5
RST_28:	jp	Main
	DS	5
RST_30:	jp	Main
	DS	5
RST_38:	jp	Main
	DS	5
	jp	irq_VBlank
	DS	5
	jp	irq_LCDC
	DS	5
	jp	irq_Timer
	DS	5
	jp	irq_Serial
	DS	5
	jp	irq_HiLo
	DS	5

	DS	$100-$68

	nop
	jp	Main

	DB	$CE,$ED,$66,$66,$CC,$0D,$00,$0B,$03,$73,$00,$83,$00,$0C,$00,$0D
	DB	$00,$08,$11,$1F,$88,$89,$00,$0E,$DC,$CC,$6E,$E6,$DD,$DD,$D9,$99
	DB	$BB,$BB,$67,$63,$6E,$0E,$EC,$CC,$DD,$DC,$99,$9F,$BB,$B9,$33,$3E

		;0123456789ABCDEF
	DB	"                "
	DB	0,0,0	;SuperGameboy
	DB	0	;CARTTYPE
			;--------
			;0 - ROM ONLY
			;1 - ROM+MBC1
			;2 - ROM+MBC1+RAM
			;3 - ROM+MBC1+RAM+BATTERY
			;5 - ROM+MBC2
			;6 - ROM+MBC2+BATTERY

	DB	0	;ROMSIZE
			;-------
			;0 - 256 kBit ( 32 kByte,  2 banks)
			;1 - 512 kBit ( 64 kByte,  4 banks)
			;2 -   1 MBit (128 kByte,  8 banks)
			;3 -   2 MBit (256 kByte, 16 banks)
			;3 -   4 MBit (512 kByte, 32 banks)

	DB	0	;RAMSIZE
			;-------
			;0 - NONE
			;1 -  16 kBit ( 2 kByte, 1 bank )
			;2 -  64 kBit ( 8 kByte, 1 bank )
			;3 - 256 kBit (32 kByte, 4 banks)

	DW	$0000	;Manufacturer

	DB	0	;Version
	DB	0	;Complement check
	DW	0	;Checksum

; --
; -- Initialize the Gameboy
; --

Main::
	; disable interrupts

	di

	; we want a stack

	ld	hl,StackTop
	ld	sp,hl

	; oh, and initialize the rest of the RAM to zero

	ld	bc,$2000-$200
	xor	a,a
	call	mem_Set

	; prepare for some interrupts

	call	irq_Init

	; We'd better make a longjump to the user's main.
	; Only the linker knows where it's been placed.
	; Even if this is a 32k image it's ok to long jump.
	; In fact it doesn't work without it at least on a
	; SSC when the image is downloaded raw without
	; the menu?!??!

	ljp	UserMain




; --
; -- Variables
; --

	SECTION	"StartupVars",BSS

Stack:	DS	$200
StackTop: