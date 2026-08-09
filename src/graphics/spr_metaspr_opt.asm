; Optimized metasprite queue routines extracted from spr_metaspr.c
	section	"DONTMERGE_text.far._SpriteEngine_AddMetaSprite.0","acrx"
	a16
	x16
	global	_SpriteEngine_AddMetaSprite

; void SpriteEngine_AddMetaSprite(__reg("a/x") struct game_object *o, __reg("r0/r1") const struct spr_metaspr_definition *m)
_SpriteEngine_AddMetaSprite:
	a16
	x16

	tax
	
	; Precalculate the origin and depth
	lda $7e000c,x
	sec
	sbc <_bg_scroll_y+2
	tay
	sec
	sbc $7e0010,x
	sta r5
	tya
	clc
	adc #16
	bpl .depth_is_positive
		lda #$0000
		bra .no_saturate_depth
.depth_is_positive:
	lsr
	cmp #$007f
	bcc .no_saturate_depth
		lda #$007f
.no_saturate_depth:
	and #$007f
	sta r3

	lda $7e0008,x ; carry is guaranteed clear
	sec
	sbc <_bg_scroll_x+2
	sta r4

.metasprite_loop:
	; Metasprite defs
	; uint16_t tileattrib; +0
	; int16_t offset_x; +2
	; int16_t offset_y; +4
	; uint16_t size; +6
	; Test if sprite queue full
	lda _spr_normal_count
	cmp #64
	bcs .finish

	asl
	asl
	asl ; clears carry
	sta r2

	; Test size
	ldy #6
	lda [r0],y

	bmi .finish
	bne .large

.small:
	lda r4
	ldy #2
	adc [r0],y
	
	bpl .x_pos
.x_neg:
	ldy r2
	cmp #-16
	bcc .next_item
	; Object partially on the left edge
	sta _spr_queue_normal,y
	lda #$40
	sta _spr_queue_normal+6,y
	clc
	bra .y_test
.x_pos:
	ldy r2
	cmp #256
	bcs .next_item
	sta _spr_queue_normal,y
	lda #$00
	sta _spr_queue_normal+6,y
.y_test:

	lda r5
	ldy #4
	adc [r0],y
	
	bpl .y_pos
.y_neg:
	cmp #-16
	bcc .next_item
	bra .draw
.y_pos:
	cmp #224
	bcs .next_item
	bra .draw

.large:
	lda r4
	ldy #2
	adc [r0],y
	
	bpl .x_pos_lg
.x_neg_lg:
	cmp #-32
	bcc .next_item
	; Object partially on the left edge
	ldy r2
	sta _spr_queue_normal,y
	lda #$c0
	sta _spr_queue_normal+6,y
	clc
	bra .y_test_lg
.x_pos_lg:
	cmp #256
	bcs .next_item
	ldy r2
	sta _spr_queue_normal,y
	lda #$80
	sta _spr_queue_normal+6,y
.y_test_lg:
	lda r5
	ldy #4
	adc [r0],y

	bpl .y_pos_lg

.y_neg_lg:
	cmp #-32
	bcc .next_item
	bra .draw
.y_pos_lg:
	cmp #224
	bcs .next_item

.draw:
	ldy r2
	sta _spr_queue_normal+2,y
	sep #$20
	a8
	lda r3
	sta _spr_queue_normal+7,y
	rep #$20
	a16
	lda [r0]
	sta _spr_queue_normal+4,y
	inc _spr_normal_count
.next_item:

	lda r0
	clc
	adc #8
	sta r0
	bra .metasprite_loop

.finish:
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_AddMetaSprite_Back.0","acrx"
	a16
	x16
	global	_SpriteEngine_AddMetaSprite_Back

; void SpriteEngine_AddMetaSprite_Back(__reg("a/x") struct game_object *o, __reg("r0/r1") const struct spr_metaspr_definition *m)
_SpriteEngine_AddMetaSprite_Back:
	a16
	x16

	tax
	
	; Precalculate the origin and depth
	lda $7e000c,x
	sec
	sbc <_bg_scroll_y+2
	tay
	sec
	sbc $7e0010,x
	sta r5
	tya
	clc
	adc #16
	bpl .depth_is_positive
		lda #$0000
		bra .no_saturate_depth
.depth_is_positive:
	lsr
	cmp #$007f
	bcc .no_saturate_depth
		lda #$007f
.no_saturate_depth:
	and #$007f
	sta r3

	lda $7e0008,x ; carry is guaranteed clear
	sec
	sbc <_bg_scroll_x+2
	sta r4

.metasprite_loop:
	; Metasprite defs
	; uint16_t tileattrib; +0
	; int16_t offset_x; +2
	; int16_t offset_y; +4
	; uint16_t size; +6
	; Test if sprite queue full
	lda _spr_back_count
	cmp #64
	bcs .finish

	asl
	asl
	asl ; clears carry
	sta r2

	; Test size
	ldy #6
	lda [r0],y

	bmi .finish
	bne .large

.small:
	lda r4
	ldy #2
	adc [r0],y
	
	bpl .x_pos
.x_neg:
	ldy r2
	cmp #-16
	bcc .next_item
	; Object partially on the left edge
	sta _spr_queue_back,y
	lda #$40
	sta _spr_queue_back+6,y
	clc
	bra .y_test
.x_pos:
	ldy r2
	cmp #256
	bcs .next_item
	sta _spr_queue_back,y
	lda #$00
	sta _spr_queue_back+6,y
.y_test:

	lda r5
	ldy #4
	adc [r0],y
	
	bpl .y_pos
.y_neg:
	cmp #-16
	bcc .next_item
	bra .draw
.y_pos:
	cmp #224
	bcs .next_item
	bra .draw

.large:
	lda r4
	ldy #2
	adc [r0],y
	
	bpl .x_pos_lg
.x_neg_lg:
	cmp #-32
	bcc .next_item
	; Object partially on the left edge
	ldy r2
	sta _spr_queue_back,y
	lda #$c0
	sta _spr_queue_back+6,y
	clc
	bra .y_test_lg
.x_pos_lg:
	cmp #256
	bcs .next_item
	ldy r2
	sta _spr_queue_back,y
	lda #$80
	sta _spr_queue_back+6,y
.y_test_lg:
	lda r5
	ldy #4
	adc [r0],y

	bpl .y_pos_lg

.y_neg_lg:
	cmp #-32
	bcc .next_item
	bra .draw
.y_pos_lg:
	cmp #224
	bcs .next_item

.draw:
	ldy r2
	sta _spr_queue_back+2,y
	sep #$20
	a8
	lda r3
	sta _spr_queue_back+7,y
	rep #$20
	a16
	lda [r0]
	sta _spr_queue_back+4,y
	inc _spr_back_count
.next_item:

	lda r0
	clc
	adc #8
	sta r0
	bra .metasprite_loop

.finish:
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
