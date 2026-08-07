	section	"DONTMERGE_text.far._ObjectSystem_MoveWithoutCollision.0","acrx"
	a16
	x16
	global	_ObjectSystem_MoveWithoutCollision
_ObjectSystem_MoveWithoutCollision:
	a16
	x16
	phb
	phx
	plb
	tax
	lda !$0006,x
	clc
	adc !$0012,x
	sta !$0006,x
	lda !$0008,x
	adc !$0014,x
	sta !$0008,x
	clc
	adc !$0028,x
	sta !$002c,x
	lda !$000a,x
	clc
	adc !$0016,x
	sta !$000a,x
	lda !$000c,x
	adc !$0018,x
	sta !$000c,x
	clc
	adc !$002a,x
	sta !$002e,x
	plb
	plb
	rtl

	section	"DONTMERGE_text.far._ObjectSystem_MoveWithoutCollision_Fast.0","acrx"
	a16
	x16
	global	_ObjectSystem_MoveWithoutCollision_Fast
_ObjectSystem_MoveWithoutCollision_Fast:
	a16
	x16
	phb
	phx
	plb
	tax
	lda !$0006,x
	clc
	adc !$0012,x
	sta !$0006,x
	lda !$0008,x
	adc !$0014,x
	sta !$0008,x
	lda !$000a,x
	clc
	adc !$0016,x
	sta !$000a,x
	lda !$000c,x
	adc !$0018,x
	sta !$000c,x
	plb
	plb
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
