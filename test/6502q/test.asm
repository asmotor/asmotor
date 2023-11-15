help .equ 87
help2 = 42

	and	#help
	
	txa
	lda	#1$
	and	#2*(3+4)*5/10\5<<2>>1
	and	#($02+8)^4|$10 & $2
	rts
