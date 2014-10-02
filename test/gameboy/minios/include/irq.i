
	IF	!DEF(IRQ_INC)
IRQ_INC	SET	1

; --
; -- Externally declared symbols
; --

	GLOBAL	irq_Init
	GLOBAL	irq_Set

	GLOBAL	irq_VBlank
	GLOBAL	irq_LCDC
	GLOBAL	irq_Timer
	GLOBAL	irq_Serial
	GLOBAL	irq_HiLo

	ENDC	;IRQ_INC