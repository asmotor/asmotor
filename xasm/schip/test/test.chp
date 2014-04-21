	SECTION	"Code",CODE[$200]

CODE_COLLISION_EXPECT_0 = 0
CODE_COLLISION_EXPECT_1 = 1
CODE_ADD_EXPECT_255 = 2
CODE_ADD_EXPECT_FLAG_0 = 3
CODE_ADD_EXPECT_0 = 4
CODE_ADD_EXPECT_FLAG_1 = 5
CODE_SUB_EXPECT_1 = 6
CODE_SUB_EXPECT_FLAG_1 = 7
CODE_SUB_EXPECT_MINUS_1 = 8
CODE_SUB_EXPECT_FLAG_0 = 9
CODE_SUBN_EXPECT_1 = 10
CODE_SUBN_EXPECT_FLAG_1 = 11
CODE_SUBN_EXPECT_MINUS_1 = 12
CODE_SUBN_EXPECT_FLAG_0 = 13
CODE_AND_EXPECT_AA = 14
CODE_OR_EXPECT_AA = 15
CODE_XOR_EXPECT_55 = 16
CODE_SHL_EXPECT_54 = 17
CODE_SHL_EXPECT_FLAG_1 = 18
CODE_SHL_EXPECT_A8 = 19
CODE_SHL_EXPECT_FLAG_0 = 20
CODE_SHR_EXPECT_2A = 21
CODE_SHR_EXPECT_FLAG_1 = 22
CODE_SHR_EXPECT_15 = 23
CODE_SHR_EXPECT_FLAG_0 = 24
CODE_BCD_EXPECT_2 = 25
CODE_BCD_EXPECT_3 = 26
CODE_BCD_EXPECT_9 = 27
CODE_LOAD_EXPECT_0 = 28
CODE_LOAD_EXPECT_1 = 29
CODE_LOAD_EXPECT_2 = 30
CODE_LOAD_EXPECT_3 = 31
CODE_LOAD_EXPECT_FF = 32
CODE_STORE_EXPECT_9_1 = 33
CODE_STORE_EXPECT_0_1 = 34
CODE_STORE_EXPECT_9_2 = 35
CODE_STORE_EXPECT_8_2 = 36
CODE_STORE_EXPECT_7_2 = 37
CODE_STORE_EXPECT_6_2 = 38

ASSERT:	MACRO
	sne		\1,\2
	jp		.ok\@
	ld		v0,\3
	jp		ShowError
.ok\@
	ENDM

	cls
	high

	call	DrawFrame
	call	TestLoad
	call	TestStore
	call	TestBCD
	call	TestCollision
	call	TestAlu

End:
	jp		End

TestLoad:
	ld		i,.data
	ld		v0,$FF
	ld		v1,$FF
	ldm		v0,(i)
	ASSERT	v0,0,CODE_LOAD_EXPECT_0
	ASSERT	v1,$FF,CODE_LOAD_EXPECT_FF
	ld		v4,$FF
	ldm		v3,(i)
	ASSERT	v0,0,CODE_LOAD_EXPECT_0
	ASSERT	v1,1,CODE_LOAD_EXPECT_1
	ASSERT	v2,2,CODE_LOAD_EXPECT_2
	ASSERT	v3,3,CODE_LOAD_EXPECT_3
	ASSERT	v4,$FF,CODE_LOAD_EXPECT_FF
	ret
	
.data
	DB		0,1,2,3

TestStore:
	ld		i,.data
	ld		v0,9
	ld		v1,8
	ldm		(i),v0
	ld		v0,0
	ld		v1,0
	ldm		v1,(i)
	ASSERT	v0,9,CODE_STORE_EXPECT_9_1
	ASSERT	v1,0,CODE_STORE_EXPECT_0_1
	ld		v0,9
	ld		v1,8
	ld		v2,7
	ld		v3,6
	ldm		(i),v3
	ld		v0,0
	ld		v1,0
	ld		v2,0
	ld		v3,0
	ldm		v3,(i)
	ASSERT	v0,9,CODE_STORE_EXPECT_9_2
	ASSERT	v1,8,CODE_STORE_EXPECT_8_2
	ASSERT	v2,7,CODE_STORE_EXPECT_7_2
	ASSERT	v3,6,CODE_STORE_EXPECT_6_2
	ret


.data
	DB		0,0,0,0

TestBCD:
	ld		v0,239
	ld		i,Digits
	bcd		v0
	ldm		v2,(i)
	ASSERT	v0,2,CODE_BCD_EXPECT_2
	ASSERT	v1,3,CODE_BCD_EXPECT_3
	ASSERT	v2,9,CODE_BCD_EXPECT_9
	ret


TestAlu:
; Test ADD
	ld		v0,128
	ld		v1,127
	add		v1,v0
	ASSERT	v1,255,CODE_ADD_EXPECT_255
	ASSERT	vf,0,CODE_ADD_EXPECT_FLAG_0
	ld		v0,128
	ld		v1,128
	add		v1,v0
	ASSERT	v1,0,CODE_ADD_EXPECT_0
	ASSERT	vf,1,CODE_ADD_EXPECT_FLAG_1

; Test SUB
	ld		v0,128
	ld		v1,127
	sub		v0,v1
	ASSERT	v0,1,CODE_SUB_EXPECT_1
	ASSERT	vf,1,CODE_SUB_EXPECT_FLAG_1
	ld		v0,128
	ld		v1,129
	sub		v0,v1
	ASSERT	v0,-1,CODE_SUB_EXPECT_MINUS_1
	ASSERT	vf,0,CODE_SUB_EXPECT_FLAG_0

; Test SUBN
	ld		v0,128
	ld		v1,127
	subn	v1,v0
	ASSERT	v1,1,CODE_SUBN_EXPECT_1
	ASSERT	vf,1,CODE_SUBN_EXPECT_FLAG_1
	ld		v0,128
	ld		v1,129
	subn	v1,v0
	ASSERT	v1,-1,CODE_SUBN_EXPECT_MINUS_1
	ASSERT	vf,0,CODE_SUBN_EXPECT_FLAG_0

; Test AND
	ld		v0,$AA
	ld		v1,$FF
	and		v1,v0
	ASSERT	v1,$AA,CODE_AND_EXPECT_AA

; Test OR
	ld		v0,$A0
	ld		v1,$0A
	or		v1,v0
	ASSERT	v1,$AA,CODE_OR_EXPECT_AA

; Test XOR
	ld		v0,$AA
	ld		v1,$FF
	xor		v1,v0
	ASSERT	v1,$55,CODE_XOR_EXPECT_55

; Test SHL
	ld		v0,$AA
	shl		v0
	ASSERT	v0,$54,CODE_SHL_EXPECT_54
	ASSERT	vf,1,CODE_SHL_EXPECT_FLAG_1
	shl		v0
	ASSERT	v0,$A8,CODE_SHL_EXPECT_A8
	ASSERT	vf,0,CODE_SHL_EXPECT_FLAG_0

; Test SHR
	ld		v0,$55
	shr		v0
	ASSERT	v0,$2A,CODE_SHR_EXPECT_2A
	ASSERT	vf,1,CODE_SHR_EXPECT_FLAG_1
	shr		v0
	ASSERT	v0,$15,CODE_SHR_EXPECT_15
	ASSERT	vf,0,CODE_SHR_EXPECT_FLAG_0

	ret

TestCollision:
	ld		v0,1
	ld		v1,1
	drw		v0,v1,1
	ASSERT	vf,0,CODE_COLLISION_EXPECT_0
	drw		v0,v1,1
	ASSERT	vf,1,CODE_COLLISION_EXPECT_1
	ret

; Shows the error code in v0 and halts execution
ShowError:
	ld		i,Digits
	bcd		v0
	ldm		v2,(i)
	ld		v3,10
	ld		v4,10
	ldf10	v0
	drw		v3,v4,10
	add		v3,9
	ldf10	v1
	drw		v3,v4,10
	add		v3,9
	ldf10	v2
	drw		v3,v4,10
	jp		End

DrawFrame:
	ld		i,SpriteDot
	ld		v0,0
	ld		v1,0
	call	.loop_x
	ld		v0,0
	ld		v1,63
	call	.loop_x
	ld		v0,0
	ld		v1,0
	call	.loop_y
	ld		v0,127
	ld		v1,0
.loop_y
	drw		v0,v1,1
	add		v1,1
	se		v1,64
	jp		.loop_y
	ret
.loop_x
	drw		v0,v1,1
	add		v0,1
	se		v0,128
	jp		.loop_x
	ret

SpriteDot:
	DB	$80

Digits:
	DSB		4

