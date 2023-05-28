	OPT	mc2

	SECTION	"Test",CODE[$0]
Start:
	brk	#$01
	ora	($11,x)
	DB	$02
	DB	$03
	tsb	$14
	ora	$12
	asl	$12
	rmb0	$12

	php
	ora	#$12
	asl
	DB	$0B
	tsb	$1234
	ora	$1234
	asl	$1234
	bbr0	$12,@

	bpl	@
	ora	($fe),y
	ora	($13)
	DB	$13
	trb	$14
	ora	$12,x
	asl	$12,x
	rmb1	$12

	clc
	ora	$1234,y
	ina
	inc	a
	DB	$1B
	trb	$1234
	ora	$1234,x
	asl	$1234,x
	bbr1	$12,@

	jsr	@
	and	($12,x)
	DB	$22
	DB	$23
	bit	$12
	and	$12
	rol	$12
	rmb	2,$12

	plp
	and	#$12
	rol
	DB	$2B
	bit	$1234
	and	$1234
	rol	$1234
	bbr2	$12,@

	bmi	@
	and	($12),y
	and	($12)
	DB	$33
	bit	$12,x
	and	$12,x
	rol	$12,x
	rmb3	$12

	sec
	and	$1234,y
	dec	a
	dea
	DB	$3B
	bit	$1234,x
	and	$1234,x
	rol	$1234,x
	bbr3	$12,@

	rti
	eor	($12,x)
	DB	$42
	DB	$43
	DB	$44
	eor	$12
	lsr	$12
	rmb4	$12

	pha
	eor	#$12
	lsr
	DB	$4B
	jmp	$1234
	eor	$1234
	lsr	$1234
	bbr	4,$12,@

	bvc	@
	eor	($12),y
	eor	($12)
	DB	$53
	DB	$54
	eor	$12,x
	lsr	$12,x
	rmb5	$12

	cli
	eor	$1234,y
	phy
	DB	$5B
	DB	$5C
	eor	$1234,x
	lsr	$1234,x
	bbr	5,$12,@

	rts
	adc	($12,x)
	DB	$62
	DB	$63
	stz	$12
	adc	$12
	ror	$12
	rmb6	$12

	pla
	adc	#$12
	ror
	DB	$6B
	jmp	($1234)
	adc	$1234
	ror	$1234
	bbr	6,$12,@

	bvs	@
	adc	($12),y
	adc	($12)
	DB	$73
	stz	$12,x
	adc	$12,x
	ror	$12,x
	rmb7	$12

	sei
	adc	$1234,y
	ply
	DB	$7B
	jmp	($1234,x)
	adc	$1234,x
	ror	$1234,x
	bbr7	$12,@

	bra	@
	sta	($12,x)
	DB	$82
	DB	$83
	sty	$12
	sta	$12
	stx	$12
	smb0	$12

	dey
	bit	#$12
	txa
	DB	$8B
	sty	$1234
	sta	$1234
	stx	$1234
	bbs0	$12,@

	bcc	@
	sta	($12),y
	sta	($12)
	DB	$93
	sty	$12,x
	sta	$12,x
	stx	$12,y
	smb1	$12

	tya
	sta	$1234,y
	txs
	DB	$9B
	stz	$1234
	sta	$1234,x
	stz	$1234,x
	bbs1	$12,@

	ldy	#$12
	lda	($12,x)
	ldx	#$12
	DB	$A3
	ldy	$12
	lda	$12
	ldx	$12
	smb2	$12

	tay
	lda	#$12
	tax
	DB	$AB
	ldy	$1234
	lda	$1234
	ldx	$1234
	bbs2	$12,@

	bcs	@
	lda	($12),y
	lda	($12)
	DB	$B3
	ldy	$12,x
	lda	$12,x
	ldx	$12,y
	smb3	$12

	clv
	lda	$1234,y
	tsx
	DB	$BB
	ldy	$1234,x
	lda	$1234,x
	ldx	$1234,y
	bbs3	$12,@

	cpy	#$12
	cmp	($12,x)
	DB	$C2
	DB	$C3
	cpy	$12
	cmp	$12
	dec	$12
	smb4	$12

	iny
	cmp	#$12
	dex
	wai
	cpy	$1234
	cmp	$1234
	dec	$1234
	bbs4	$12,@

	bne	@
	cmp	($12),y
	cmp	($12)
	DB	$D3
	DB	$D4
	cmp	$12,x
	dec	$12,x
	smb5	$12

	cld
	cmp	$1234,y
	phx
	stp
	DB	$DC
	cmp	$1234,x
	dec	$1234,x
	bbs5	$12,@

	cpx	#$12
	sbc	($12,x)
	DB	$E2
	DB	$E3
	cpx	$12
	sbc	$12
	inc	$12
	smb6	$12

	inx
	sbc	#$12
	nop
	DB	$EB
	cpx	$1234
	sbc	$1234
	inc	$1234
	bbs6	$12,@

	beq	@
	sbc	($12),y
	sbc	($12)
	DB	$F3
	DB	$F4
	sbc	$12,x
	inc	$12,x
	smb7	$12

	sed
	sbc	$1234,y
	plx
	DB	$FB
	DB	$FC
	sbc	$1234,x
	inc	$1234,x
	bbs7	$12,@


