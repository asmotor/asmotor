;***************************************************************************
;*
;* MAIN.ASM - Standard ROM-image header
;*
;* All fields left to zero since RGBFix does a nice job of filling them in
;* in for us...
;*
;***************************************************************************

	INCLUDE	"HARDWARE.INC"

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

.wait	ldh	a,[rLY]
	cp	144
	jr	nc,.wait
	ld	a,0
	ldh	[rLCDC],a

	ld	hl,Font
	ld	de,$8000
	ld	bc,16*8*2
	call	mem_Copy

	ld	a,0
	ldh	[rSCY],a
	ldh	[rSCX],a

	ld	a,STATF_LYC
	ldh	[rSTAT],a

	ld	a,80
	ldh	[rLYC],a

	ld	a,%00000011
	ldh	[rBGP],a

	ld	a,IEF_VBLANK	;|IEF_LCDC
	ldh	[rIE],a

	ld	a,LCDCF_ON|LCDCF_BG8000|LCDCF_BG9800|LCDCF_BGON
	ldh	[rLCDC],a

	ei

dummy:	jp	dummy

irq_VBlank:	pop	hl
	ld	a,h
	and	a,$f
	ld	[$9800+1],a
	ld	a,h
	swap	a
	and	a,$f
	ld	[$9800+0],a
	ld	a,l
	and	a,$f
	ld	[$9800+3],a
	ld	a,l
	swap	a
	and	a,$f
	ld	[$9800+2],a
	ld	hl,$1000
	push	hl
	reti

irq_LCDC:	reti
irq_Timer:	reti
irq_Serial:	reti
irq_HiLo:	reti

lcd_DisplayOff::	ld	a,[_HW+rIE]
	push	af
	and	a,~IEF_VBLANK
	ld	[_HW+rIE],a
	ld	a,[_HW+rLCDC]
	and	a,(~LCDCF_ON)&$FF
	ld	[_HW+rLCDC],a
	pop	af
	ld	[_HW+rIE],a
	ret

;***************************************************************************
;*
;* mem_Copy - "Copy" a memory region
;*
;* input:
;*   hl - pSource
;*   de - pDest
;*   bc - bytecount
;*
;***************************************************************************
mem_Copy::
	inc	b
	inc	c
	jr	.skip
.loop	ld	a,[hl+]
	ld	[de],a
	inc	de
.skip	dec	c
	jr	nz,.loop
	dec	b
	jr	nz,.loop
	ret

Font:
	DW	`01111100
	DW	`11111110
	DW	`11000110
	DW	`11000110
	DW	`11000110
	DW	`11111110
	DW	`01111100
	DW	`00000000

	DW	`00011000
	DW	`00011000
	DW	`00011000
	DW	`00011000
	DW	`00011000
	DW	`00011000
	DW	`00011000
	DW	`00000000

	DW	`01111100
	DW	`11111110
	DW	`00000110
	DW	`01111110
	DW	`11100000
	DW	`11111110
	DW	`11111110
	DW	`00000000

	DW	`01111100
	DW	`11111110
	DW	`00001110
	DW	`00011100
	DW	`00001110
	DW	`11111110
	DW	`01111100
	DW	`00000000

	DW	`11000110
	DW	`11000110
	DW	`11111110
	DW	`01111110
	DW	`00000110
	DW	`00000110
	DW	`00000110
	DW	`00000000

	DW	`11111110
	DW	`11111110
	DW	`11000000
	DW	`11111100
	DW	`00000110
	DW	`11111110
	DW	`11111100
	DW	`00000000

	DW	`01111110
	DW	`11111110
	DW	`11000000
	DW	`11111100
	DW	`11000110
	DW	`11111110
	DW	`01111100
	DW	`00000000

	DW	`11111110
	DW	`11111110
	DW	`00000110
	DW	`00001100
	DW	`00011000
	DW	`00011000
	DW	`00011000
	DW	`00000000

	DW	`01111100
	DW	`11111110
	DW	`11000110
	DW	`01111110
	DW	`11000110
	DW	`11111110
	DW	`01111100
	DW	`00000000

	DW	`01111100
	DW	`11111110
	DW	`11000110
	DW	`01111110
	DW	`00000110
	DW	`11111110
	DW	`01111100
	DW	`00000000

	DW	`01111100
	DW	`11111110
	DW	`11000110
	DW	`11111110
	DW	`11000110
	DW	`11000110
	DW	`11000110
	DW	`00000000

	DW	`11111100
	DW	`11111110
	DW	`11000110
	DW	`11111100
	DW	`11000110
	DW	`11111110
	DW	`11111100
	DW	`00000000

	DW	`01111100
	DW	`11111110
	DW	`11000000
	DW	`11000000
	DW	`11000000
	DW	`11111110
	DW	`01111100
	DW	`00000000

	DW	`11111100
	DW	`11111110
	DW	`11000110
	DW	`11000110
	DW	`11000110
	DW	`11111110
	DW	`11111100
	DW	`00000000

	DW	`11111110
	DW	`11111110
	DW	`11000000
	DW	`11111000
	DW	`11000000
	DW	`11111110
	DW	`11111110
	DW	`00000000

	DW	`11111110
	DW	`11111110
	DW	`11000000
	DW	`11111000
	DW	`11000000
	DW	`11000000
	DW	`11000000
	DW	`00000000

	SECTION	"Nops",HOME[$1000]

	REPT	$3000
	nop
	ENDR

	SECTION	"Nops2",CODE
	REPT	$3000
	nop
	ENDR


; --
; -- Variables
; --

	SECTION	"StartupVars",BSS

Stack:	DS	$200
StackTop: