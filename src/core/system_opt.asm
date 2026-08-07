	section	"DONTMERGE_text.far._System_GetInput.0","acrx"
	a16
	x16
	global	_System_GetInput
_System_GetInput:
.input_wait:
	lda $4212
	and #$01
	bne .input_wait
	lda _input_pad0
	sta r0
	lda $4218
	ora $421a
	sta _input_pad0
	eor r0
	and _input_pad0
	sta _input_pad0_new
	rtl

	section	"DONTMERGE_text.far._System_GetInput_Manual.0","acrx"
	a16
	x16
	global	_System_GetInput_Manual
_System_GetInput_Manual:
	a8
	sep #$20

	lda #$01
	sta $4016
	stz $4016

	sta r1
	stz r1+1
	sta r2
	stz r2+1

.input_read_1:
	lda $4016
	lsr
	rol r1
	rol r1+1
	bcc .input_read_1

.input_read_2:
	lda $4017
	lsr
	rol r2
	rol r2+1
	bcc .input_read_2

	a16
	rep #$20

	lda _input_pad0
	sta r0
	lda r1
	ora r2
	sta _input_pad0
	eor r0
	and _input_pad0
	sta _input_pad0_new
	rtl

	section	"DONTMERGE_text.far._System_AlignToVblank.0","acrx"
	a16
	x16
	global	_System_AlignToVblank
_System_AlignToVblank:
	a8
	sep #$20
	bit $4212
	bpl .phase_2
.phase_1:
	bit $4212
	bmi .phase_1
.phase_2:
	bit $4212
	bpl .phase_2
	a16
	rep #$20
	rtl

	section	"DONTMERGE_text.far._System_Hsync.0","acrx"
	a16
	x16
	global	_System_Hsync
_System_Hsync:
	a16
	sta $4207

	a8
	sep #$24

	lda _shadow_nmitimen
	ora #$10
	sta $4200

	wai

	bit $4211

	lda _shadow_nmitimen
	sta $4200

	a16
	rep #$24
	rtl

	section	"DONTMERGE_text.far._System_CheckForActiveDisplayEnd.0","acrx"
	a16
	x16
	global	_System_CheckForActiveDisplayEnd
_System_CheckForActiveDisplayEnd:
	a8
	sep #$20
	bit $2137

	lda $213d
	sta r0
	bit $213d

	lda _system_use_long_vblank
	beq .normal

	lda r0
	cmp #208
	bne .end
.fblank_loop:
	bit $213f
	bit $2137
	lda $213d
	bit $213d
	cmp #208
	beq .fblank_loop
	cmp #209
	beq .fblank_loop
	bra .end

.normal:
	lda r0
	cmp #224
	bne .end

.normal_loop:
	bit $213f
	bit $2137
	lda $213d
	bit $213d
	cmp #224
	beq .normal_loop
	cmp #225
	beq .normal_loop

.end:
	bit $213f
	a16
	rep #$20
	rtl

	section	"DONTMERGE_text.far._System_CopyBlock.0","acrx"
	a16
	x16
	global	_System_CopyBlock
_System_CopyBlock:
	phb

	tax
	a8
	sep #$20
	lda r3
	sta >_system_MVNCodeInWRAM+1
	lda r1
	sta >_system_MVNCodeInWRAM+2
	a16
	rep #$20
	txa
	dec
	ldx r0
	ldy r2
	jsl >_system_MVNCodeInWRAM;

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
