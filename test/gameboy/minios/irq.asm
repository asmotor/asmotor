;***************************************************************************
;*
;* IRQ.ASM - Routines for interrupts
;*
;* irq_Init - Initialize interrupt system
;* irq_Set - Set an interrupt pointer
;*
;***************************************************************************

	INCLUDE	"hardware.i"
	INCLUDE	"utility.i"

	SECTION	"InterruptControl",HOME




;***************************************************************************
;*
;* irq_Init - Initialize interrupt system
;*
;***************************************************************************
irq_Init::
	xor	a,a
	ld	[_HW+rIF],a
	ld	[_HW+rIE],a
	ei
	ret




;***************************************************************************
;*
;* irq_Set - Set an interrupt pointer
;*
;* input:
;*   a - interrupt bit:	0 - VBlank
;*		1 - LCDC
;*		2 - Timer
;*		3 - Serial
;*		4 - HiLo
;*   b - bank#
;*  de - pointer, NULL==disable irq
;*
;***************************************************************************
irq_Set::
	; save <b:de> in the right pointer

	; c=a*3
	ld	h,a
	sla	a
	add	a,h
	ld	c,a
	ld	a,h

	; hl=&pVBlank[a*3]
	ld	hl,pVBlank
	push	bc
	ld	b,0
	add	hl,bc
	pop	bc

	; c=a
	ld	c,a

	ld	a,b
	ld	[hl+],a
	ld	a,e
	ld	[hl+],a
	ld	a,d
	ld	[hl+],a

	; find mask
	ld	hl,.masks
	ld	b,0
	add	hl,bc
	ld	c,[hl]
	ld	a,[_HW+rIE]
	ld	b,a
	xor	a,a
	cp	a,d
	jr	nz,.ptr_ok
	cp	a,e
	jr	nz,.ptr_ok

	; NULL pointer, disable interrupt

	ld	a,c
	cpl
	and	a,b
	ld	[_HW+rIE],a
	ret

	; OK pointer, enable interrupt

.ptr_ok	ld	a,c
	or	a,b
	ld	[_HW+rIE],a
	ret

.masks	DB	IEF_VBLANK
	DB	IEF_LCDC
	DB	IEF_TIMER
	DB	IEF_SERIAL
	DB	IEF_HILO




; --
; -- Internal interrupt handling routines
; --

irq_VBlank::	pusha
	ld	hl,pVBlank
	ld	b,~IEF_VBLANK
	jp	irq_Common

irq_LCDC::	pusha
	ld	hl,pLCDC
	ld	b,~IEF_LCDC
	jp	irq_Common

irq_Timer::	pusha
	ld	hl,pTimer
	ld	b,~IEF_TIMER
	jp	irq_Common

irq_Serial::	pusha
	ld	hl,pSerial
	ld	b,~IEF_SERIAL
	jp	irq_Common

irq_HiLo::	pusha
	ld	hl,pHiLo
	ld	b,~IEF_HILO

; -- fallthrough to irq_Common

irq_Common:	ld	a,[hl+]
	ld	c,a	;c=bank#
	ld	a,[hl+]
	ld	d,a
	ld	a,[hl+]
	ld	h,a
	ld	l,d	;hl=address
	ld	a,[_HW+rIF]
	and	a,b
	ld	[_HW+rIF],a
	ld	a,c
	call	z80_LongCall
irq_Return:	popa
	ei
	reti




; --
; -- Variables
; --

	SECTION	"InterruptVars",BSS

pVBlank:	DS	3
pLCDC:	DS	3
pTimer:	DS	3
pSerial:	DS	3
pHiLo:	DS	3