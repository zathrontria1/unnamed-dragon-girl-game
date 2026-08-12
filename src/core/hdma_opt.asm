	section	"DONTMERGE_text.far._HdmaEngine_UpdateBgScrollValues.0","acrx"
	a16
	x16
	global	_HdmaEngine_UpdateBgScrollValues
_HdmaEngine_UpdateBgScrollValues:
	lda _hdma_scroll_select
	inc
	and #$0001
	sta r0
	beq .dest_table_0
		ldx #64
		bra .dest_table_done
.dest_table_0:
	ldx #0
.dest_table_done:

	lda _obj_player_active_fireballs
	lsr
	lsr
	cmp #16
	bcc .fireball_in_limit
		lda #15

.fireball_in_limit:
	xba
	lsr
	sta r1
	lda _hdma_scroll_sine_index
	asl
	adc r1
	adc #<_const_hdma_scroll_sine
	sta r5
	lda #^_const_hdma_scroll_sine
	sta r6

	lda _bg_scroll_y+2
	dec
	sta r2

	lda >_gfx_enable_heatwave
	and #$00ff
	bne .apply_heatwave_scroll

	lda r2
	sta >_hdma_scroll_data+0,x
	sta >_hdma_scroll_data+2,x
	sta >_hdma_scroll_data+4,x
	sta >_hdma_scroll_data+6,x
	sta >_hdma_scroll_data+8,x
	sta >_hdma_scroll_data+10,x
	sta >_hdma_scroll_data+12,x
	sta >_hdma_scroll_data+14,x
	sta >_hdma_scroll_data+16,x
	sta >_hdma_scroll_data+18,x
	sta >_hdma_scroll_data+20,x
	sta >_hdma_scroll_data+22,x
	sta >_hdma_scroll_data+24,x
	sta >_hdma_scroll_data+26,x
	sta >_hdma_scroll_data+28,x
	sta >_hdma_scroll_data+30,x
	sta >_hdma_scroll_data+32,x
	sta >_hdma_scroll_data+34,x
	sta >_hdma_scroll_data+36,x
	sta >_hdma_scroll_data+38,x
	sta >_hdma_scroll_data+40,x
	sta >_hdma_scroll_data+42,x
	sta >_hdma_scroll_data+44,x
	sta >_hdma_scroll_data+46,x
	sta >_hdma_scroll_data+48,x
	sta >_hdma_scroll_data+50,x
	sta >_hdma_scroll_data+52,x
	sta >_hdma_scroll_data+54,x
	sta >_hdma_scroll_data+56,x
	sta >_hdma_scroll_data+58,x
	sta >_hdma_scroll_data+60,x
	sta >_hdma_scroll_data+62,x
	bra .scroll_values_done

.apply_heatwave_scroll:
	clc
	ldy #0
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+0,x

	ldy #2
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+2,x

	ldy #4
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+4,x

	ldy #6
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+6,x

	ldy #8
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+8,x

	ldy #10
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+10,x

	ldy #12
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+12,x

	ldy #14
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+14,x

	ldy #16
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+16,x

	ldy #18
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+18,x

	ldy #20
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+20,x

	ldy #22
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+22,x

	ldy #24
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+24,x

	ldy #26
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+26,x

	ldy #28
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+28,x

	ldy #30
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+30,x

	ldy #32
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+32,x

	ldy #34
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+34,x

	ldy #36
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+36,x

	ldy #38
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+38,x

	ldy #40
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+40,x

	ldy #42
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+42,x

	ldy #44
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+44,x

	ldy #46
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+46,x

	ldy #48
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+48,x

	ldy #50
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+50,x

	ldy #52
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+52,x

	ldy #54
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+54,x

	ldy #56
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+56,x

	ldy #58
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+58,x

	ldy #60
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+60,x

	ldy #62
	lda r2
	adc [r5],y
	sta >_hdma_scroll_data+62,x

.scroll_values_done:

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
	stz r9
	stz r10
	stz r11

	lda >_hdma_coldata_usegradient
	and #$00ff
	beq .coldata_check_last
	lda >_gfx_enable_heatwave
	and #$00ff
	beq .coldata_check_last

	lda >_gfx_cmath_r
	lsr
	lsr
	lsr
	lsr
	lsr
	sta r9

	lda >_gfx_cmath_g
	lsr
	lsr
	lsr
	lsr
	lsr
	sta r10

	lda >_gfx_cmath_b
	lsr
	lsr
	lsr
	lsr
	lsr
	sta r11

.coldata_check_last:
	lda r9
	cmp >_hdma_coldata_last_r
	bne .coldata_do_update
	lda r10
	cmp >_hdma_coldata_last_g
	bne .coldata_do_update
	lda r11
	cmp >_hdma_coldata_last_b
	beq .coldata_early_exit

.coldata_do_update:
	lda r9
	sta >_hdma_coldata_last_r
	lda r10
	sta >_hdma_coldata_last_g
	lda r11
	sta >_hdma_coldata_last_b

	lda _hdma_coldata_select
	inc
	and #$0001
	sta r0
	beq .coldata_dest_0
		ldx #256
		bra .coldata_dest_done
.coldata_dest_0:
	ldx #0
.coldata_dest_done:

	lda r9
	ora r10
	ora r11
	bne .start_coldata

.coldata_is_zero:
	lda #$0000
	sta >_hdma_coldata_data+0,x
	sta >_hdma_coldata_data+2,x
	sta >_hdma_coldata_data+4,x
	sta >_hdma_coldata_data+8,x
	sta >_hdma_coldata_data+10,x
	sta >_hdma_coldata_data+12,x
	sta >_hdma_coldata_data+16,x
	sta >_hdma_coldata_data+18,x
	sta >_hdma_coldata_data+20,x
	sta >_hdma_coldata_data+24,x
	sta >_hdma_coldata_data+26,x
	sta >_hdma_coldata_data+28,x
	sta >_hdma_coldata_data+32,x
	sta >_hdma_coldata_data+34,x
	sta >_hdma_coldata_data+36,x
	sta >_hdma_coldata_data+40,x
	sta >_hdma_coldata_data+42,x
	sta >_hdma_coldata_data+44,x
	sta >_hdma_coldata_data+48,x
	sta >_hdma_coldata_data+50,x
	sta >_hdma_coldata_data+52,x
	sta >_hdma_coldata_data+56,x
	sta >_hdma_coldata_data+58,x
	sta >_hdma_coldata_data+60,x
	sta >_hdma_coldata_data+64,x
	sta >_hdma_coldata_data+66,x
	sta >_hdma_coldata_data+68,x
	sta >_hdma_coldata_data+72,x
	sta >_hdma_coldata_data+74,x
	sta >_hdma_coldata_data+76,x
	sta >_hdma_coldata_data+80,x
	sta >_hdma_coldata_data+82,x
	sta >_hdma_coldata_data+84,x
	sta >_hdma_coldata_data+88,x
	sta >_hdma_coldata_data+90,x
	sta >_hdma_coldata_data+92,x
	sta >_hdma_coldata_data+96,x
	sta >_hdma_coldata_data+98,x
	sta >_hdma_coldata_data+100,x
	sta >_hdma_coldata_data+104,x
	sta >_hdma_coldata_data+106,x
	sta >_hdma_coldata_data+108,x
	sta >_hdma_coldata_data+112,x
	sta >_hdma_coldata_data+114,x
	sta >_hdma_coldata_data+116,x
	sta >_hdma_coldata_data+120,x
	sta >_hdma_coldata_data+122,x
	sta >_hdma_coldata_data+124,x
	sta >_hdma_coldata_data+128,x
	sta >_hdma_coldata_data+130,x
	sta >_hdma_coldata_data+132,x
	sta >_hdma_coldata_data+136,x
	sta >_hdma_coldata_data+138,x
	sta >_hdma_coldata_data+140,x
	sta >_hdma_coldata_data+144,x
	sta >_hdma_coldata_data+146,x
	sta >_hdma_coldata_data+148,x
	sta >_hdma_coldata_data+152,x
	sta >_hdma_coldata_data+154,x
	sta >_hdma_coldata_data+156,x
	sta >_hdma_coldata_data+160,x
	sta >_hdma_coldata_data+162,x
	sta >_hdma_coldata_data+164,x
	sta >_hdma_coldata_data+168,x
	sta >_hdma_coldata_data+170,x
	sta >_hdma_coldata_data+172,x
	sta >_hdma_coldata_data+176,x
	sta >_hdma_coldata_data+178,x
	sta >_hdma_coldata_data+180,x
	sta >_hdma_coldata_data+184,x
	sta >_hdma_coldata_data+186,x
	sta >_hdma_coldata_data+188,x
	sta >_hdma_coldata_data+192,x
	sta >_hdma_coldata_data+194,x
	sta >_hdma_coldata_data+196,x
	sta >_hdma_coldata_data+200,x
	sta >_hdma_coldata_data+202,x
	sta >_hdma_coldata_data+204,x
	sta >_hdma_coldata_data+208,x
	sta >_hdma_coldata_data+210,x
	sta >_hdma_coldata_data+212,x
	sta >_hdma_coldata_data+216,x
	sta >_hdma_coldata_data+218,x
	sta >_hdma_coldata_data+220,x
	sta >_hdma_coldata_data+224,x
	sta >_hdma_coldata_data+226,x
	sta >_hdma_coldata_data+228,x
	sta >_hdma_coldata_data+232,x
	sta >_hdma_coldata_data+234,x
	sta >_hdma_coldata_data+236,x
	sta >_hdma_coldata_data+240,x
	sta >_hdma_coldata_data+242,x
	sta >_hdma_coldata_data+244,x
	sta >_hdma_coldata_data+248,x
	sta >_hdma_coldata_data+250,x
	sta >_hdma_coldata_data+252,x
	bra .end_coldata_write

.start_coldata:
	lda #$2000
	clc
	sta >_hdma_coldata_data+248,x
	adc r9
	sta >_hdma_coldata_data+240,x
	adc r9
	sta >_hdma_coldata_data+232,x
	adc r9
	sta >_hdma_coldata_data+224,x
	adc r9
	sta >_hdma_coldata_data+216,x
	adc r9
	sta >_hdma_coldata_data+208,x
	adc r9
	sta >_hdma_coldata_data+200,x
	adc r9
	sta >_hdma_coldata_data+192,x
	adc r9
	sta >_hdma_coldata_data+184,x
	adc r9
	sta >_hdma_coldata_data+176,x
	adc r9
	sta >_hdma_coldata_data+168,x
	adc r9
	sta >_hdma_coldata_data+160,x
	adc r9
	sta >_hdma_coldata_data+152,x
	adc r9
	sta >_hdma_coldata_data+144,x
	adc r9
	sta >_hdma_coldata_data+136,x
	adc r9
	sta >_hdma_coldata_data+128,x
	adc r9
	sta >_hdma_coldata_data+120,x
	adc r9
	sta >_hdma_coldata_data+112,x
	adc r9
	sta >_hdma_coldata_data+104,x
	adc r9
	sta >_hdma_coldata_data+96,x
	adc r9
	sta >_hdma_coldata_data+88,x
	adc r9
	sta >_hdma_coldata_data+80,x
	adc r9
	sta >_hdma_coldata_data+72,x
	adc r9
	sta >_hdma_coldata_data+64,x
	adc r9
	sta >_hdma_coldata_data+56,x
	adc r9
	sta >_hdma_coldata_data+48,x
	adc r9
	sta >_hdma_coldata_data+40,x
	adc r9
	sta >_hdma_coldata_data+32,x
	adc r9
	sta >_hdma_coldata_data+24,x
	adc r9
	sta >_hdma_coldata_data+16,x
	adc r9
	sta >_hdma_coldata_data+8,x
	adc r9
	sta >_hdma_coldata_data+0,x

	lda #$4000
	clc
	sta >_hdma_coldata_data+250,x
	adc r10
	sta >_hdma_coldata_data+242,x
	adc r10
	sta >_hdma_coldata_data+234,x
	adc r10
	sta >_hdma_coldata_data+226,x
	adc r10
	sta >_hdma_coldata_data+218,x
	adc r10
	sta >_hdma_coldata_data+210,x
	adc r10
	sta >_hdma_coldata_data+202,x
	adc r10
	sta >_hdma_coldata_data+194,x
	adc r10
	sta >_hdma_coldata_data+186,x
	adc r10
	sta >_hdma_coldata_data+178,x
	adc r10
	sta >_hdma_coldata_data+170,x
	adc r10
	sta >_hdma_coldata_data+162,x
	adc r10
	sta >_hdma_coldata_data+154,x
	adc r10
	sta >_hdma_coldata_data+146,x
	adc r10
	sta >_hdma_coldata_data+138,x
	adc r10
	sta >_hdma_coldata_data+130,x
	adc r10
	sta >_hdma_coldata_data+122,x
	adc r10
	sta >_hdma_coldata_data+114,x
	adc r10
	sta >_hdma_coldata_data+106,x
	adc r10
	sta >_hdma_coldata_data+98,x
	adc r10
	sta >_hdma_coldata_data+90,x
	adc r10
	sta >_hdma_coldata_data+82,x
	adc r10
	sta >_hdma_coldata_data+74,x
	adc r10
	sta >_hdma_coldata_data+66,x
	adc r10
	sta >_hdma_coldata_data+58,x
	adc r10
	sta >_hdma_coldata_data+50,x
	adc r10
	sta >_hdma_coldata_data+42,x
	adc r10
	sta >_hdma_coldata_data+34,x
	adc r10
	sta >_hdma_coldata_data+26,x
	adc r10
	sta >_hdma_coldata_data+18,x
	adc r10
	sta >_hdma_coldata_data+10,x
	adc r10
	sta >_hdma_coldata_data+2,x

	lda #$8000
	clc
	sta >_hdma_coldata_data+252,x
	adc r11
	sta >_hdma_coldata_data+244,x
	adc r11
	sta >_hdma_coldata_data+236,x
	adc r11
	sta >_hdma_coldata_data+228,x
	adc r11
	sta >_hdma_coldata_data+220,x
	adc r11
	sta >_hdma_coldata_data+212,x
	adc r11
	sta >_hdma_coldata_data+204,x
	adc r11
	sta >_hdma_coldata_data+196,x
	adc r11
	sta >_hdma_coldata_data+188,x
	adc r11
	sta >_hdma_coldata_data+180,x
	adc r11
	sta >_hdma_coldata_data+172,x
	adc r11
	sta >_hdma_coldata_data+164,x
	adc r11
	sta >_hdma_coldata_data+156,x
	adc r11
	sta >_hdma_coldata_data+148,x
	adc r11
	sta >_hdma_coldata_data+140,x
	adc r11
	sta >_hdma_coldata_data+132,x
	adc r11
	sta >_hdma_coldata_data+124,x
	adc r11
	sta >_hdma_coldata_data+116,x
	adc r11
	sta >_hdma_coldata_data+108,x
	adc r11
	sta >_hdma_coldata_data+100,x
	adc r11
	sta >_hdma_coldata_data+92,x
	adc r11
	sta >_hdma_coldata_data+84,x
	adc r11
	sta >_hdma_coldata_data+76,x
	adc r11
	sta >_hdma_coldata_data+68,x
	adc r11
	sta >_hdma_coldata_data+60,x
	adc r11
	sta >_hdma_coldata_data+52,x
	adc r11
	sta >_hdma_coldata_data+44,x
	adc r11
	sta >_hdma_coldata_data+36,x
	adc r11
	sta >_hdma_coldata_data+28,x
	adc r11
	sta >_hdma_coldata_data+20,x
	adc r11
	sta >_hdma_coldata_data+12,x
	adc r11
	sta >_hdma_coldata_data+4,x

.end_coldata_write:
	lda r0
	sta _hdma_coldata_select
	beq .coldata_no_offset
		lda #$0243
.coldata_no_offset:

	clc
	adc #<_hdma_coldata_tables
	sta _hdma_coldata_ptr
.coldata_early_exit:
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
