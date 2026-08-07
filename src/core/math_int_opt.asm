	section	"DONTMERGE_text.far._Math_GetRandom_u16.0","acrx"
	a16
	x16
	global	_Math_GetRandom_u16
_Math_GetRandom_u16:
	a8
	sep #$20
	lda _rand_array
	asl
	eor _rand_array+1
	sta _rand_array+1
	rol
	eor _rand_array+2
	sta _rand_array+2
	eor _rand_array
	sta _rand_array
	lda _rand_array+1
	ror
	eor _rand_array+2
	sta _rand_array+2
	eor _rand_array+1
	sta _rand_array+1
	a16
	rep #$30
	lda _rand_array
	rtl

	zpage	r0
	zpage	r1
	zpage	r2
	zpage	r3
	zpage	r4
	zpage	r5
	zpage	r6
	zpage	r7
	zpage	r8
	zpage	r9
	zpage	r10
	zpage	r11
	zpage	r12
	zpage	r13
	zpage	r14
	zpage	r15
	zpage	r16
	zpage	r17
	zpage	r18
	zpage	r19
	zpage	r20
	zpage	r21
	zpage	r22
	zpage	r23
	zpage	r24
	zpage	r25
	zpage	r26
	zpage	r27
	zpage	r28
	zpage	r29
	zpage	r30
	zpage	r31
	zpage	btmp0
	zpage	btmp1
	zpage	btmp2
	zpage	btmp3
