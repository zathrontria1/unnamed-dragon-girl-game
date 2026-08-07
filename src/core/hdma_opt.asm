	section	"DONTMERGE_text.far._HdmaEngine_UpdateBgScrollValues.0","acrx"
	a16
	x16
	global	_HdmaEngine_UpdateBgScrollValues
_HdmaEngine_UpdateBgScrollValues:
	lda _hdma_scroll_select
	inc
	and #$0001
	sta r0
	xba
	lsr
	lsr
	adc #<_hdma_scroll_data
	sta r3
	lda #^_hdma_scroll_data
	sta r4

	lda _obj_player_active_fireballs
	lsr
	lsr
	cmp #16
	bcc .fireball_in_limit
		lda #15

.fireball_in_limit:
	xba
	lsr
	sta r5
	lda _hdma_scroll_sine_index
	asl
	adc r5
	adc #<_const_hdma_scroll_sine
	sta r5
	lda #^_const_hdma_scroll_sine
	sta r6

	lda _bg_scroll_y+2
	dec
	tax

	ldy #$003e

.loop:
	txa
	clc
	adc [r5],y
	sta [r3],y
	dey
	dey
	bpl .loop

	lda r0
	sta _hdma_scroll_select
	beq .scroll_no_offset
		lda #24
.scroll_no_offset:

	clc
	adc #<_hdma_scroll_tables
	sta _hdma_scroll_ptr

	lda _hdma_scroll_sine_index
	inc ; (1 * V_MUL) >> 1, where V_MUL=2 at FPS=30
	and #$001f
	sta _hdma_scroll_sine_index
	rtl

	section	"DONTMERGE_text.far._HdmaEngine_UpdateColdataValues.0","acrx"
	a16
	x16
	global	_HdmaEngine_UpdateColdataValues
_HdmaEngine_UpdateColdataValues:
	lda _hdma_coldata_select
	inc
	and #$0001
	sta r0
	xba
	adc #<_hdma_coldata_data
	sta r3
	adc #2
	sta r5
	adc #2
	sta r7
	lda #^_hdma_coldata_data
	sta r4
	sta r6
	sta r8

	lda _hdma_coldata_usegradient
	and #$00ff
	beq .coldata_is_zero

	lda _gfx_cmath_r
	lsr
	lsr
	lsr
	lsr
	lsr
	sta r9

	lda _gfx_cmath_g
	lsr
	lsr
	lsr
	lsr
	lsr
	sta r10

	lda _gfx_cmath_b
	lsr
	lsr
	lsr
	lsr
	lsr
	sta r11

	lda r9
	ora r10
	ora r11
	bne .start_coldata

.coldata_is_zero:
	ldy #$00fc
	lda #$0000

.loop_coldata_zero:
	sta [r3],y
	dey
	dey
	bpl .loop_coldata_zero
	bra .end_coldata_write

.start_coldata:
	lda #$2000
	sta r12
	lda #$4000
	sta r13
	lda #$8000
	sta r14

	ldy #$00f8

.loop_coldata:
	lda r12
	clc
	adc r9
	sta [r3], y
	sta r12

	lda r13
	adc r10
	sta [r5], y
	sta r13

	lda r14
	adc r11
	sta [r7], y
	sta r14

	tya
	sec
	sbc #8
	tay
	bpl .loop_coldata

.end_coldata_write:
	lda r0
	sta _hdma_coldata_select
	beq .coldata_no_offset
		lda #$02a3
.coldata_no_offset:

	clc
	adc #<_hdma_coldata_tables
	sta _hdma_coldata_ptr
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
