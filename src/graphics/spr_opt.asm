	section	"DONTMERGE_text.far._SpriteEngine_AddToFrontLayer.0","acrx"
	a16
	x16
	global	_SpriteEngine_AddToFrontLayer

; void SpriteEngine_AddToFrontLayer(__reg("a/x") struct game_object * o, uint16_t tileattrib)
_SpriteEngine_AddToFrontLayer:
	a16
	x16

	tax

	; Tile attribute data is 6th item in stack
	; object X position is 6th byte
	; object Y position is 10th byte
	; object Z position is 14th byte
	; (use only high 16 bits for them)

	; Test if queue full first
	lda _spr_front_count
	cmp #64
	bcs .reject

	asl
	asl
	asl
	asl
	tay

	lda $7e0008,x
	sec
	sbc <_bg_scroll_x+2
	bpl .x_pos

.x_neg:
	cmp #-16
	bcc .reject
	; Object partially on the left edge
	sta _spr_queue_front,y
	lda #$40
	sta _spr_queue_front+6,y
	bra .y_test
.x_pos:
	cmp #256
	bcs .reject
	sta _spr_queue_front,y
	lda #$00
	sta _spr_queue_front+6,y
.y_test:
	lda $7e000c,x
	sec
	sbc $7e0010,x
	sec
	sbc <_bg_scroll_y+2
	bpl .y_pos

.y_neg:
	cmp #-16
	bcc .reject
	clc
	bra .finish
.y_pos:
	cmp #224
	bcs .reject

.finish:
	sta _spr_queue_front+2,y
	adc #16
	and #$00ff
	sta _spr_queue_front+8,y
	lda 4,s
	sta _spr_queue_front+4,y
	inc _spr_front_count

.reject:
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_AddToSortedLayer.0","acrx"
	a16
	x16
	global	_SpriteEngine_AddToSortedLayer

; void SpriteEngine_AddToSortedLayer(__reg("a/x") struct game_object * o, uint16_t tileattrib)
_SpriteEngine_AddToSortedLayer:
	a16
	x16
	tax

	; Tile attribute data is 6th item in stack
	; object X position is 6th byte
	; object Y position is 10th byte
	; object Z position is 14th byte
	; (use only high 16 bits for them)

	; Test if queue full first
	lda _spr_normal_count
	cmp #64
	bcs .reject

	asl
	asl
	asl
	asl
	tay

	lda $7e0008,x
	sec
	sbc <_bg_scroll_x+2
	bpl .x_pos

.x_neg:
	cmp #-16
	bcc .reject
	; Object partially on the left edge
	sta _spr_queue_normal,y
	lda #$40
	sta _spr_queue_normal+6,y
	bra .y_test
.x_pos:
	cmp #256
	bcs .reject
	sta _spr_queue_normal,y
	lda #$00
	sta _spr_queue_normal+6,y
.y_test:
	lda $7e000c,x
	sec
	sbc $7e0010,x
	sec
	sbc <_bg_scroll_y+2
	sec
	sbc #2
	bpl .y_pos

.y_neg:
	cmp #-16
	bcc .reject
	clc
	bra .finish
.y_pos:
	cmp #224
	bcs .reject

.finish:
	sta _spr_queue_normal+2,y
	adc #16
	and #$00ff
	sta _spr_queue_normal+8,y
	lda 4,s
	sta _spr_queue_normal+4,y
	inc _spr_normal_count

.reject:
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_AddToBackLayer.0","acrx"
	a16
	x16
	global	_SpriteEngine_AddToBackLayer

; void SpriteEngine_AddToBackLayer(__reg("a/x") struct game_object * o, uint16_t tileattrib)
_SpriteEngine_AddToBackLayer:
	a16
	x16
	tax

	; Tile attribute data is 6th item in stack
	; object X position is 6th byte
	; object Y position is 10th byte
	; object Z position is 14th byte
	; (use only high 16 bits for them)

	; Test if queue full first
	lda _spr_back_count
	cmp #64
	bcs .reject

	asl
	asl
	asl
	asl
	tay

	lda $7e0008,x
	sec
	sbc <_bg_scroll_x+2
	bpl .x_pos

.x_neg:
	cmp #-16
	bcc .reject
	; Object partially on the left edge
	sta _spr_queue_back,y
	lda #$40
	sta _spr_queue_back+6,y
	bra .y_test
.x_pos:
	cmp #256
	bcs .reject
	sta _spr_queue_back,y
	lda #$00
	sta _spr_queue_back+6,y
.y_test:
	lda $7e000c,x
	sec
	sbc $7e0010,x
	sec
	sbc <_bg_scroll_y+2
	bpl .y_pos

.y_neg:
	cmp #-16
	bcc .reject
	clc
	bra .finish
.y_pos:
	cmp #224
	bcs .reject

.finish:
	sta _spr_queue_back+2,y
	adc #16
	and #$00ff
	sta _spr_queue_back+8,y
	lda 4,s
	sta _spr_queue_back+4,y
	inc _spr_back_count

.reject:
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_ProcessSpriteLists_WriteFrontSprites.0","acrx"
	a16
	x16
	global	_SpriteEngine_ProcessSpriteLists_WriteFrontSprites

; void SpriteEngine_ProcessSpriteLists_WriteFrontSprites()
_SpriteEngine_ProcessSpriteLists_WriteFrontSprites:
	a16
	x16

	lda _spr_front_count
	beq .end_drawfront

	tay
	lda #<_spr_queue_front
	sta r0
	lda #^_spr_queue_front
	sta r1

.loop_drawfrontsprites:
	jsl >_SpriteEngine_DrawSprite
	lda r0
	clc
	adc #16
	sta r0
	dey
	bne .loop_drawfrontsprites

.end_drawfront:
	stz _spr_front_count
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_ProcessSpriteLists_ClearDepthBuffer.0","acrx"
	a16
	x16
	global	_SpriteEngine_ProcessSpriteLists_ClearDepthBuffer

; void SpriteEngine_ProcessSpriteLists_ClearDepthBuffer()
_SpriteEngine_ProcessSpriteLists_ClearDepthBuffer:
	a16
	x16

	x8
	sep #$10

	phd
	lda #<_spr_depth_count
	and #$ff00
	pha
	pld
	tax
	clc
.loop_depthclear:
	stz <_spr_depth_count,x
	stz <_spr_depth_count+2,x
	stz <_spr_depth_count+4,x
	stz <_spr_depth_count+6,x
	stz <_spr_depth_count+8,x
	stz <_spr_depth_count+10,x
	stz <_spr_depth_count+12,x
	stz <_spr_depth_count+14,x
	txa
	adc #16
	tax
	bne .loop_depthclear
	stz !_spr_depth_count+255
	pld
	rep #$10
	x16
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_ProcessSpriteLists_TallySprites.0","acrx"
	a16
	x16
	global	_SpriteEngine_ProcessSpriteLists_TallySprites

; void SpriteEngine_ProcessSpriteLists_TallySprites()
_SpriteEngine_ProcessSpriteLists_TallySprites:
	a16
	x16

	lda #<_spr_queue_normal
	sta r0

	lda _spr_normal_count
	beq .end
	sta r2

	lda #$0000
	ldy #8

	a8
	sep #$20
	phb
	lda #^_spr_depth_count
	pha
	plb
	clc

.loop_depthtally:
	lda (r0),y
	tax
	inx
	inc !_spr_depth_count,x
	lda r0
	adc #16
	sta r0
	bcc .depthtally_nocarry
	clc
	inc r0+1
.depthtally_nocarry:
	dec r2
	bne .loop_depthtally

	a16
	rep #$20
	plb

.end:
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_ProcessSpriteLists_CalculateOffsets.0","acrx"
	a16
	x16
	global	_SpriteEngine_ProcessSpriteLists_CalculateOffsets

; void SpriteEngine_ProcessSpriteLists_CalculateOffsets()
_SpriteEngine_ProcessSpriteLists_CalculateOffsets:
	a16
	x16

	phd
	lda #<_spr_depth_count
	and #$ff00
	pha
	pld
	x8
	sep #$10
	lda #$0000
	tax
	lda _spr_sprite_count
	clc
	adc _spr_normal_count
	sep #$21
	a8
	tay
.loop_oamoffsetcalc:
	tya
	sec
	sta <_spr_depth_count,x
	sbc <_spr_depth_count+1,x
	sta <_spr_depth_count+1,x
	sbc <_spr_depth_count+2,x
	sta <_spr_depth_count+2,x
	sbc <_spr_depth_count+3,x
	sta <_spr_depth_count+3,x
	sbc <_spr_depth_count+4,x
	sta <_spr_depth_count+4,x
	sbc <_spr_depth_count+5,x
	sta <_spr_depth_count+5,x
	sbc <_spr_depth_count+6,x
	sta <_spr_depth_count+6,x
	sbc <_spr_depth_count+7,x
	sta <_spr_depth_count+7,x
	sbc <_spr_depth_count+8,x
	sta <_spr_depth_count+8,x
	sbc <_spr_depth_count+9,x
	sta <_spr_depth_count+9,x
	sbc <_spr_depth_count+10,x
	sta <_spr_depth_count+10,x
	sbc <_spr_depth_count+11,x
	sta <_spr_depth_count+11,x
	sbc <_spr_depth_count+12,x
	sta <_spr_depth_count+12,x
	sbc <_spr_depth_count+13,x
	sta <_spr_depth_count+13,x
	sbc <_spr_depth_count+14,x
	sta <_spr_depth_count+14,x
	sbc <_spr_depth_count+15,x
	sta <_spr_depth_count+15,x
	sbc <_spr_depth_count+16,x
	tay
	txa
	clc
	adc #16
	tax
	bne .loop_oamoffsetcalc
	tya
	sta !_spr_depth_count+256
	rep #$30
	a16
	x16
	pld
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_ProcessSpriteLists_WriteSortedSprites.0","acrx"
	a16
	x16
	global	_SpriteEngine_ProcessSpriteLists_WriteSortedSprites

; void SpriteEngine_ProcessSpriteLists_WriteSortedSprites()
_SpriteEngine_ProcessSpriteLists_WriteSortedSprites:
	a16
	x16

	lda _spr_normal_count
	beq .end2
	sta r2

	ldy #0
.loop_spritewrite:
	; Decrement the depth count
	; Wipe the accumulator first
	tdc
	sep #$20
	a8
	lda !_spr_queue_normal+8,y
	tax

	lda >_spr_depth_count,x
	dec
	sta >_spr_depth_count,x

	; Prepare the indices
	tax

	rep #$20
	a16
	asl
	asl
	sta r3
	sep #$20
	a8

	; Transfer the sprite information
	lda !_spr_queue_normal+6,y
	sta >_shadow_oam+512,x

	ldx r3

	lda !_spr_queue_normal+2,y
	sta >_shadow_oam+1,x

	lda !_spr_queue_normal,y
	sta >_shadow_oam,x

	rep #$21
	a16
	lda !_spr_queue_normal+4,y
	sta >_shadow_oam+2,x

	tya
	adc #16
	tay

	dec r2
	bne .loop_spritewrite

.end2:
	lda _spr_sprite_count
	clc
	adc _spr_normal_count
	sta _spr_sprite_count
	stz _spr_normal_count
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_ProcessSpriteLists_WriteBackSprites.0","acrx"
	a16
	x16
	global	_SpriteEngine_ProcessSpriteLists_WriteBackSprites

; void SpriteEngine_ProcessSpriteLists_WriteBackSprites()
_SpriteEngine_ProcessSpriteLists_WriteBackSprites:
	a16
	x16

	lda _spr_back_count
	beq .end_drawback

	tay
	lda #<_spr_queue_back
	sta r0
	lda #^_spr_queue_back
	sta r1

.loop_drawbacksprites:
	jsl >_SpriteEngine_DrawSprite
	lda r0
	clc
	adc #16
	sta r0
	dey
	bne .loop_drawbacksprites

.end_drawback:
	stz _spr_back_count
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_DrawSprite.0","acrx"
	a16
	x16
	global	_SpriteEngine_DrawSprite

; void SpriteEngine_DrawSprite(__reg("r0/r1") struct spr_queue_entry * s)
_SpriteEngine_DrawSprite:
	a16
	x16
	phy

	lda _spr_sprite_count
	asl
	asl
	tax

	ldy r0

	lda $0004,y
	sta >_shadow_oam+2,x
	sep #$20
	a8
	lda $0000,y
	sta >_shadow_oam,x
	lda $0002,y
	sta >_shadow_oam+1,x
	ldx _spr_sprite_count
	lda $0006,y
	sta >_shadow_oam+512,x
	inx
	stx _spr_sprite_count
	a16
	rep #$20
	ply
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_PackOamHighTable.0","acrx"
	a16
	x16
	global	_SpriteEngine_PackOamHighTable

; void SpriteEngine_PackOamHighTable()
_SpriteEngine_PackOamHighTable:
	a16
	x16
	phd
	lda #<_shadow_oam+512
	and #$ff00
	pha
	pld

	sep #$31
	a8
	x8
	ldx #0
	txy
	clc
.loop_packoam:
	lda <_shadow_oam+512,x
	lsr
	lsr
	ora <_shadow_oam+513,x
	lsr
	lsr
	ora <_shadow_oam+514,x
	lsr
	lsr
	ora <_shadow_oam+515,x
	sta _shadow_oam+512,y

	lda <_shadow_oam+516,x
	lsr
	lsr
	ora <_shadow_oam+517,x
	lsr
	lsr
	ora <_shadow_oam+518,x
	lsr
	lsr
	ora <_shadow_oam+519,x
	sta _shadow_oam+513,y

	lda <_shadow_oam+520,x
	lsr
	lsr
	ora <_shadow_oam+521,x
	lsr
	lsr
	ora <_shadow_oam+522,x
	lsr
	lsr
	ora <_shadow_oam+523,x
	sta _shadow_oam+514,y

	lda <_shadow_oam+524,x
	lsr
	lsr
	ora <_shadow_oam+525,x
	lsr
	lsr
	ora <_shadow_oam+526,x
	lsr
	lsr
	ora <_shadow_oam+527,x
	sta _shadow_oam+515,y

	txa
	adc #16
	tax
	lsr
	lsr
	tay
	cpy #32
	bcc .loop_packoam
	a16
	x16
	rep #$30

	pld
	stz _spr_sprite_count
	rtl

	section	"DONTMERGE_text.far._SpriteEngine_ResetOam.0","acrx"
	a16
	x16
	global	_SpriteEngine_ResetOam

; void SpriteEngine_ResetOam()
_SpriteEngine_ResetOam:
	a16
	x16

	lda _spr_sprite_count_prev
	bit #3
	beq .sprcount_is_already_multiple_of_four

.loop_roundcount:
	inc
	bit #3
	bne .loop_roundcount

.sprcount_is_already_multiple_of_four:
	sta r10

	lda _spr_sprite_count
	cmp r10
	bcs .end_sprreset

	tay

	asl
	asl
	tax

	lda #^(_shadow_oam+512)
	sta r2+2
	lda #<_shadow_oam+512
	sta r2

	sep #$20
	a8
.loop_sprreset:
	lda #0
	sta [r2],y
	sta >_shadow_oam,x
	lda #240
	sta >_shadow_oam+1,x

	inx
	inx
	inx
	inx

	iny
	cpy r10

	bcc .loop_sprreset

	a16
	rep #$20

.end_sprreset:
	lda _spr_sprite_count
	sta _spr_sprite_count_prev
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
