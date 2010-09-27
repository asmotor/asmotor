;***************************************************************************
;*
;* UTILITY.ASM - Utility routines
;*
;* mem_Set - "Set" a memory region
;* mem_Copy - "Copy" a memory region
;*
;* fade_Init - Initialize a fadestruct for fading
;* fade_Decrease - Decrease fd_ubIntensity
;* fade_Increase - Increase fd_ubIntensity
;* fade_Process - Calculate palette register
;*
;* gbi_Show - Show a .gbi file
;* gbi_VBlank - Call this in your vblank if you're showing a .gbi file
;*
;* lcd_WaitVRAM - Wait for VRAM access
;* lcd_DisplayOff - Turn off LCD display
;*
;* z80_SetBank - Set the current bank
;* z80_LongCall - Make a long distance call
;* z80_LongJp - Make a long distance jump
;*
;* pad_Read - Read the joypad
;*
;***************************************************************************

	INCLUDE	"hardware.i"
	INCLUDE	"irq.i"

	SECTION	"Utility",HOME




;***************************************************************************
;*
;* mem_Set - "Set" a memory region
;*
;* input:
;*    a - value
;*   hl - pMem
;*   bc - bytecount
;*
;***************************************************************************
mem_Set::
	inc	b
	inc	c
	jr	.skip
.loop	ld	[hl+],a
.skip	dec	c
	jr	nz,.loop
	dec	b
	jr	nz,.loop
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




;***************************************************************************
;*
;* fade_Init - Initialize a fadestruct for fading
;*
;* input:
;*   hl - pointer fadestruct+fd_Fade0
;*
;***************************************************************************
fade_Init::
	xor	a,a
	ld	[hl+],a
	ld	[hl+],a
	ld	[hl+],a
	ld	[hl+],a
	ret

;***************************************************************************
;*
;* fade_Decrease - Decrease fd_ubIntensity
;*
;* input:
;*   hl - fadestruct
;*
;* output:
;*   a - new level (0-$40)
;*
;***************************************************************************
fade_Decrease::
	ld	a,[hl]
	cp	a,0
	ret	z
	dec	a
	ld	[hl],a
	ret

;***************************************************************************
;*
;* fade_Increase - Increase fd_ubIntensity
;*
;* input:
;*   hl - fadestruct
;*
;* output:
;*   a - new level (0-$40)
;*
;***************************************************************************
fade_Increase::
	ld	a,[hl]
	cp	a,$40
	ret	z
	inc	a
	ld	[hl],a
	ret

;***************************************************************************
;*
;* fade_Process - Calculate palette register
;*
;* input:
;*   hl - fadestruct
;*
;* output:
;*    a - new palette register
;*
;***************************************************************************
fade_Process::
	ld	a,[hl+]
	ld	c,a
	ld	a,[hl+]
	ld	b,a

	call	.fadecalcone
	ld	e,a
	rrc	e
	rrc	e

	REPT	3
	srl	b
	srl	b
	call	.fadecalcone
	or	a,e
	ld	e,a
	rrc	e
	rrc	e
	ENDR

	ld	a,e
	ret

.fadecalcone
	ld	a,b
	and	a,%00000011
	ld	d,a
; calculates a=d*c
	xor	a,a
	bit	0,d
	jr	z,.skipadd
	add	a,c
.skipadd	bit	1,d
	jr	z,.skipadd2
	add	a,c
	add	a,c
.skipadd2
	ld	d,a
	rlc	d
	rlc	d
	and	a,$3F
	add	a,[hl]
	bit	6,a
	jr	z,.exit
	and	a,$3F
	ld	[hl+],a
	ld	a,%01
	add	a,d
	and	a,%11
	ret
.exit	ld	[hl+],a
	ld	a,d
	and	a,%11
	ret




;***************************************************************************
;*
;* gbi_Show - Show a .gbi file
;*
;* input:
;*   hl - image
;*
;***************************************************************************
gbi_Show::
	push	hl
	call	lcd_DisplayOff
	call	lcd_WaitVRAM
	ld	hl,$FE00
	ld	bc,$00A0
	xor	a,a
	call	mem_Set
	pop	hl

	ld	bc,40*4
	ld	de,$FE00
	call	mem_Copy

	ld	bc,40*32
	ld	de,$8000
	call	mem_Copy

	ld	a,[hl+]
	push	hl
	ld	hl,$9800
	ld	bc,32*32
	call	mem_Set
	pop	hl

	ld	a,16
	ld	de,$9800+32
.nextline	ld	bc,15
	push	af
	call	mem_Copy
	pop	af
	push	hl
	ld	h,d
	ld	l,e
	ld	de,32-15
	add	hl,de
	ld	d,h
	ld	e,l
	pop	hl
	dec	a
	jr	nz,.nextline

	ld	a,[hl+]
	ld	c,a
	ld	a,[hl+]
	ld	b,a
	ld	de,$8800
	call	mem_Copy

	ret

;***************************************************************************
;*
;* gbi_VBlank - Call this in your vblank if you're showing a .gbi file
;*
;* input:
;*   a - palette
;*
;***************************************************************************
gbi_VBlank::
	ld	[_HW+rBGP],a
	ld	[_HW+rOBP0],a
	ld	[_HW+rOBP1],a
	ld	a,LCDCF_ON|LCDCF_BG8800|LCDCF_BG9800|LCDCF_BGON|LCDCF_OBJ16|LCDCF_OBJON
	ld	[_HW+rLCDC],a
	xor	a,a
	ld	[_HW+rSCX],a
	ld	[_HW+rSCY],a
	ret




;***************************************************************************
;*
;* lcd_WaitVRAM - Wait for VRAM access
;*
;***************************************************************************
lcd_WaitVRAM::
	ld	a,[_HW+rSTAT]
	bit     1,a
	jr	nz,lcd_WaitVRAM
	ret

;***************************************************************************
;*
;* lcd_DisplayOff - Turn off LCD display
;*
;***************************************************************************
lcd_DisplayOff::
	ld	a,[_HW+rIE]
	push	af
	and	a,~IEF_VBLANK
	ld	[_HW+rIE],a
.wait	ld	a,[_HW+rLY]
	cp	144
	jr	nc,.wait
	ld	a,[_HW+rLCDC]
	and	a,(~LCDCF_ON)&$FF
	ld	[_HW+rLCDC],a
	pop	af
	ld	[_HW+rIE],a
	ret




;***************************************************************************
;*
;* z80_SetBank - Set the current bank
;*
;* input:
;*   a  - bank
;*
;***************************************************************************
z80_SetBank::
	ld	[_CurrentBank],a
	ld	[$2100],a
	ret

;***************************************************************************
;*
;* z80_LongCall - Make a long distance call
;*
;* input:
;*   hl - address
;*   a  - bank
;*
;***************************************************************************
z80_LongCall::
	ld	b,a
	ld	a,[_CurrentBank]
	push	af
	ld	a,b
	call	z80_SetBank
	ld	bc,.return
	push	bc
	jp	[hl]
.return	pop	af
	call	z80_SetBank
	ret

;***************************************************************************
;*
;* z80_LongJp - Make a long distance jump
;*
;* input:
;*   hl - address
;*   a  - bank
;*
;***************************************************************************
z80_LongJp::
	call	z80_SetBank
	jp	[hl]




;***************************************************************************
;*
;* pad_Read - Read the joypad
;*
;* output:
;*   _PadData     - joypad matrix
;*   _PadDataEdge - edge data: which buttons were pressed since last time
;*                  this routine was called
;*
;***************************************************************************
pad_Read::
	ld	a,P1F_5
	ld	[_HW+rP1],a
	ld	a,[_HW+rP1]
	ld	a,[_HW+rP1]
	cpl
	and	a,$0F
	swap	a
	ld	b,a

	ld	a,P1F_4
	ld	[_HW+rP1],a
	ld	a,[_HW+rP1]
	ld	a,[_HW+rP1]
	ld	a,[_HW+rP1]
	ld	a,[_HW+rP1]
	ld	a,[_HW+rP1]
	ld	a,[_HW+rP1]
	cpl
	and	a,$0F
	or	a,b
	ld	b,a

	ld	a,[_PadData]
	xor	a,b
	and	a,b
	ld	[_PadDataEdge],a
	ld	a,b
	ld	[_PadData],a

	ld	a,P1F_5|P1F_4
	ld	[_HW+rP1],a
	ret




; --
; -- Variables
; --

	SECTION	"UtilityVars",BSS
_winy:	DS	1
_CurrentBank:	DS	1
_PadData::	DS	1
_PadDataEdge::	DS	1