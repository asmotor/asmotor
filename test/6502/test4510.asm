	OPT mc4

	SECTION "Test",CODE[0]

	; $00-$07
	brk
	ora	($11,x)
	cle
	see
	tsb	$14
	ora	$12
	asl	$12
	rmb0	$12

	; $08-$0F
	php
	ora	#$12
	asl
	tsy
	tsb	$1234
	ora	$1234
	asl	$1234
	bbr0	$12,@

	; $10-$17
	bpl	@
	ora	($fe),y
	ora	($12),z
	lbpl	@
	trb	$14
	ora	$12,x
	asl	$12,x
	rmb1	$12

	; $18-$1F
	clc
	ora	$1234,y
	ina
	inc	a
	inz
	trb	$1234
	ora	$1234,x
	asl	$1234,x
	bbr1	$12,@

	; $20-$27
	jsr	@
	and	($12,x)
	jsr	($1234)
	jsr	($1234,x)
	bit	$12
	and	$12
	rol	$12
	rmb	2,$12

	; $28-$2F
	plp
	and	#$12
	rol
	tys
	bit	$1234
	and	$1234
	rol	$1234
	bbr2	$12,@

	; $30-$37
	bmi	@
	and	($12),y
	and	($12),z
	lbmi @
	bit	$12,x
	and	$12,x
	rol	$12,x
	rmb3	$12

	; $38-$3F
	sec
	and	$1234,y
	dec	a
	dea
	dez
	bit	$1234,x
	and	$1234,x
	rol	$1234,x
	bbr3	$12,@

	; $40-$47
	rti
	eor	($12,x)
	neg
	asr
	asr	$12
	eor	$12
	lsr	$12
	rmb4	$12

	; $48-$4F
	pha
	eor	#$12
	lsr
	taz
	jmp	$1234
	eor	$1234
	lsr	$1234
	bbr	4,$12,@

	; $50-$57
	bvc	@
	eor	($12),y
	eor	($12),z
	lbvc	@
	asr	$12,x
	eor	$12,x
	lsr	$12,x
	rmb5	$12

	; $58-$5F
	cli
	eor	$1234,y
	phy
	tab
	map
	eor	$1234,x
	lsr	$1234,x
	bbr	5,$12,@

	; $60-$67
	rts
	adc	($12,x)
	rts	#4
	bsr	@
	stz	$12
	adc	$12
	ror	$12
	rmb6	$12

	; $68-$6F
	pla
	adc	#$12
	ror
	tza
	jmp	($1234)
	adc	$1234
	ror	$1234
	bbr	6,$12,@

	; $70-$77
	bvs	@
	adc	($12),y
	adc	($12),z
	lbvs @
	stz	$12,x
	adc	$12,x
	ror	$12,x
	rmb7	$12

	; $78-$7F
	sei
	adc	$1234,y
	ply
	tba
	jmp	($1234,x)
	adc	$1234,x
	ror	$1234,x
	bbr7	$12,@

	; $80-$87
	bra	@
	sta	($12,x)
	sta ($12,sp),y
	lbra @
	sty	$12
	sta	$12
	stx	$12
	smb0	$12

	; $88-$8F
	dey
	bit	#$12
	txa
	sty $1234,x
	sty	$1234
	sta	$1234
	stx	$1234
	bbs0	$12,@

	; $90-$97
	bcc	@
	sta	($12),y
	sta	($12),z
	lbcc @
	sty	$12,x
	sta	$12,x
	stx	$12,y
	smb1	$12

	; $98-$9F
	tya
	sta	$1234,y
	txs
	stx $1234,y
	stz	$1234
	sta	$1234,x
	stz	$1234,x
	bbs1	$12,@

	; $A0-$A7
	ldy	#$12
	lda	($12,x)
	ldx	#$12
	ldz #$12
	ldy	$12
	lda	$12
	ldx	$12
	smb2	$12

	; $A8-$AF
	tay
	lda	#$12
	tax
	ldz $1234
	ldy	$1234
	lda	$1234
	ldx	$1234
	bbs2	$12,@

	; $B0-$B7
	bcs	@
	lda	($12),y
	lda	($12),z
	lbcs @
	ldy	$12,x
	lda	$12,x
	ldx	$12,y
	smb3	$12

	; $B8-$BF
	clv
	lda	$1234,y
	tsx
	ldz $1234,x
	ldy	$1234,x
	lda	$1234,x
	ldx	$1234,y
	bbs3	$12,@

	; $C0-$C7
	cpy	#$12
	cmp	($12,x)
	cpz #$12
	dew $12
	cpy	$12
	cmp	$12
	dec	$12
	smb4	$12

	; $C8-$CF
	iny
	cmp	#$12
	dex
	asw $1234
	cpy	$1234
	cmp	$1234
	dec	$1234
	bbs4	$12,@

	; $D0-$D7
	bne	@
	cmp	($12),y
	cmp	($12),z
	lbne	@
	cpz	$12
	cmp	$12,x
	dec	$12,x
	smb5	$12

	; $D8-$DF
	cld
	cmp	$1234,y
	phx
	phz
	cpz	$1234
	cmp	$1234,x
	dec	$1234,x
	bbs5	$12,@

	; $E0-$E7
	cpx	#$12
	sbc	($12,x)
	lda	($12,sp),y
	inw	$12
	cpx	$12
	sbc	$12
	inc	$12
	smb6	$12

	; $E8-$EF
	inx
	sbc	#$12
	eom
	row	$1234
	cpx	$1234
	sbc	$1234
	inc	$1234
	bbs6	$12,@

	; $F0-$F7
	beq	@
	sbc	($12),y
	sbc	($12),z
	lbeq	@
	phw	#$1234
	sbc	$12,x
	inc	$12,x
	smb7	$12

	; $F8-$FF
	sed
	sbc	$1234,y
	plx
	plz
	phw	$1234
	sbc	$1234,x
	inc	$1234,x
	bbs7	$12,@


; --
; -- 45GS02
; --

	OPT	mc5

	adc	[$12],z
	adcq	$12
	adcq	$1234
	adcq	($12)
	adcq	[$12]

	and	[$12],z
	andq	$12
	andq	$1234
	andq	($12)
	andq	[$12]

	aslq	$12
	aslq	q
	aslq	$1234
	aslq	$12,x
	aslq	$1234,x

	asrq	q
	asrq	$12
	asrq	$12,x

	bitq	$12
	bitq	$1234

	cmp	[$12],z

	cmpq	$12
	cmpq	$1234
	cmpq	($12)
	cmpq	[$12]

	deq	q
	deq	$12
	deq	$1234
	deq	$12,x
	deq	$1234,x

	eor	[$12],z

	eorq	$12
	eorq	$1234
	eorq	($12)
	eorq	[$12]

	inq	q
	inq	$12
	inq	$1234
	inq	$12,x
	inq	$1234,x

	lda	[$12],z

	ldq	$12
	ldq	$1234
	ldq	($12),z
	ldq	[$12],z

	lsrq	$12
	lsrq	q
	lsrq	$1234
	lsrq	$12,x
	lsrq	$1234,x

	ora	[$12],z

	orq	$12
	orq	$1234
	orq	($12)
	orq	[$12]

	rolq	$12
	rolq	q
	rolq	$1234
	rolq	$12,x
	rolq	$1234,x

	rorq	$12
	rorq	q
	rorq	$1234
	rorq	$12,x
	rorq	$1234,x

	sbc	[$12],z

	sbcq	$12
	sbcq	$1234
	sbcq	($12)
	sbcq	[$12]

	sta	[$12],z

	stq	$12
	stq	$1234
	stq	($12)
	stq	[$12]


    IF 0
	SETBP	$500
	lda	[$512],z
	lda	($512)
	lda	$512
	lda	$512,x
	ldx	$512,y
	lda	($512,x)
	lda	($512),y
	lda	[$512]
	lda	[$512],z
    ENDC


