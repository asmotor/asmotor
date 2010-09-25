	SECTION	"SectTest",HOME

Ting	EQU	$FE00

	ld	a,["AA"]

	IMPORT	ImportTest

RecursiveArgs:	MACRO
	IF	\1>0
	RecursiveArgs \1-1
	ENDC
	PRINTV	\1
	PRINTT	"\n"
	ENDM

john:	MACRO
	IF	johnlabel<5
johnlabel	SET	johnlabel+1
	PRINTT	"John!\n"
	john
	ENDC
	ENDM

TestMacro:	MACRO
johnlabel	SET	0
Label_In_Macro	adc	a,$10
	ld	hl,\1
	john
	ENDM

	PRINTT	"This is a test\n"

	IF	2+2==5
	PRINTT	"2+2==5\n"
	ELSE
	PRINTT	"2+2!=5\n"
		IF	2+2!=4
		PRINTT	"2+2!=4\n"
		ELSE
		PRINTT	"2+2==4\n"
		ENDC
	ENDC

	PRINTT	"Yet another test\n"
label
label2:
globallabel::
.locallabel
.locallabel2:

EQUTest	EQU	8

SETTest	SET	2
SETTest	SET	4

;testcomment:
; .test

	PRINTT	"Print the numbers $0-$5:\n"
	RecursiveArgs 5
	PRINTT	"...done!\n\n"
	and	a,ImportTest
	adc	a,$10
	adc	$20
	adc	b
	adc	a,c

	add	a,$10
	add	$10
	add	a,b
	add	[hl]
	add	hl,bc
	add	de
	add	sp,$40
.john
	and	a,$10
	and	$20
	and	b
	and	a,c
newscope:

	ld	[newscope+.john],a

	bit	0,a
.john
	call	$245
	call	nz,$1234
	call	c,$4321
newscope2:
	ccf

	cp	a,newscope2
	cp	$20
	cp	b
	cp	a,c

	cpl

	daa

	PRINTV	@-newscope2

	INCLUDE	"inctest.inc"

	dec	a
	dec	[hl]
	dec	bc

	di
	ei

	ex	hl,[sp]
	ex	[sp],hl

	halt

	inc	a
	inc	[hl]
	inc	bc

	jp	[hl]
             	jp	$245
	jp	nz,$1234
	jp	c,$4321

	jr	.weee
	jr	nc,.weee
.weee
	ld	[$3987],sp
	ld	[$FF00+c],a
	ld	[c],a
	ld	[$9876],a
	ld	[$ff76],a
	ld	[bc],a
	ld	[hl+],a
	ld	a,[$FF00+c]
	ld	a,[$9876]
	ld	a,[$ff76]
	ld	a,[de]
	ld	a,[hl-]
	ld	a,50
	ld	hl,[sp+6]
	ld	hl,[sp-6]
	ld	sp,hl
	ld	[hl],34
	ld	c,34
	ld	a,e
	ld	h,l
	ld	[hl],a
	ld	b,[hl]
	ld	hl,$1234

	nop

	or	a,$10
	or	$20
	or	b
	or	a,c

	push	af

	pop	af

	res	0,a

	ret
	ret	c
	reti

	rl	c
	rl	[hl]
	rla
	rlc	c
	rlc	d
	rlca
	rr	c
	rr	[hl]
	rra
	rrc	c
	rrc	d
	rrca

	rst	$10

	sbc	a,$10
	sbc	$20
	sbc	b
	sbc	a,c

	scf

	set	0,a

	sla	c
	sla	[hl]
	sra	c
	sra	[hl]
	srl	c
	srl	[hl]

	stop

	sub	a,$10
	sub	$20
	sub	b
	sub	a,c

	swap	c
	swap	[hl]

	xor	a,$10
	xor	$20
	xor	b
	xor	a,c

	IF	2==3
	ELSE
	ENDC
	;END