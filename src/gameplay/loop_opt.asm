	section	"DONTMERGE_text.far._Loop_Subscreen_MapDisplay_InitBackground.0","acrx"
	a16
	x16
	global	_Loop_Subscreen_MapDisplay_InitBackground
_Loop_Subscreen_MapDisplay_InitBackground:
	a8
	x16
	sep #$20

	lda #$00
	sta $2115
	ldx #$4c00
	stx $2116

	ldx #256
	stx r0

	lda #$08
	sta $4300

	ldx #<r0
	stx $4302
	lda #^r0
	sta $4304

	ldx #1024
	stx $4305

	lda #$18
	sta $4301

	lda #$01
	sta $420b

	lda #$80
	sta $2115
	ldx #$4c00
	stx $2116

	ldx #256
	stx r0

	lda #$08
	sta $4300

	ldx #<r0+1
	stx $4302
	lda #^r0
	sta $4304

	ldx #1024
	stx $4305

	lda #$19
	sta $4301

	lda #$01
	sta $420b

	a16
	x16
	rep #$30
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
