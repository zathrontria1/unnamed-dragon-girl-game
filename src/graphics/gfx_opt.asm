	section	"DONTMERGE_text.far._Gfx_ProcessMosaic.0","acrx"
	a16
	x16
	global	_Gfx_ProcessMosaic
_Gfx_ProcessMosaic:
	lda _gfx_enable_hitblur
	bne .process_mosaic
	stz _gfx_mosaic_intensity
	stz _gfx_mosaic_change
	stz _gfx_mosaic_layers
	sep #$20
	a8
	stz _shadow_mosaic
	rep #$20
	a16
	rtl

.process_mosaic:
	lda _gfx_mosaic_change
	beq .check_intensity

	xba
	and #$ff00
	clc
	adc _gfx_mosaic_intensity
	sta _gfx_mosaic_intensity

	bpl .check_upper_clamp

	stz _gfx_mosaic_intensity
	lda _gfx_mosaic_change
	bpl .check_intensity
	stz _gfx_mosaic_change
	bra .check_intensity

.check_upper_clamp:
	cmp #$0f01
	bcc .check_intensity

	lda #$0f00
	sta _gfx_mosaic_intensity
	lda _gfx_mosaic_change
	bmi .check_intensity
	stz _gfx_mosaic_change

.check_intensity:
	lda _gfx_mosaic_intensity
	bne .apply_mosaic

	sep #$20
	a8
	stz _shadow_mosaic
	rep #$20
	a16
	stz _gfx_mosaic_layers
	rtl

.apply_mosaic:
	and #$0f00
	sec
	sbc #$0100
	lsr A
	lsr A
	lsr A
	lsr A
	and #$00f0
	ora _gfx_mosaic_layers
	sep #$20
	a8
	sta _shadow_mosaic
	rep #$30
	a16
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
