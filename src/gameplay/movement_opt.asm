	section	"DONTMERGE_text.far._ObjectSystem_Move.0","acrx"
	a16
	x16
	global	_ObjectSystem_Move
_ObjectSystem_Move:
	a16
	x16
	phb
	phx
	plb
	tax

	; Fast path: check if delta.x == 0 and delta.y == 0
	lda !$0012,x
	ora !$0014,x
	ora !$0016,x
	ora !$0018,x
	bne .asm_move_active
	plb
	plb
	lda #$0000
	rtl

.asm_move_active:
	; Store object pointer offset (X) into scratch r0
	stx r0

	; Check has_x (delta_x != 0)
	lda !$0012,x
	ora !$0014,x
	beq .skip_move_x

	; temp_xl.a = pos.x.a + delta.x.a
	lda !$0006,x
	clc
	adc !$0012,x
	sta r1				; temp_xl low
	lda !$0008,x
	adc !$0014,x
	sta r2				; temp_xl high (pixel X)

	; temp_x_pixel = temp_xl.lh.h + 1
	inc
	bpl .x_pixel_pos
	lda #$0000
	bra .x_pixel_done
.x_pixel_pos:
	cmp >_map_extent_x
	bcc .x_pixel_done
	lda >_map_extent_x
	dec
.x_pixel_done:
	sta r3				; r3 = temp_x_pixel

	; temp_y = pos.y.lh.h + 1
	lda !$000c,x
	inc
	bpl .y_pixel_pos
	lda #$0000
	bra .y_pixel_done
.y_pixel_pos:
	cmp >_map_extent_y
	bcc .y_pixel_done
	lda >_map_extent_y
	dec
.y_pixel_done:
	sta r4				; r4 = temp_y

	; if (delta_x > 0)
	lda !$0014,x
	bmi .x_not_right
	bne .x_is_right
	lda !$0012,x
	beq .x_not_right
.x_is_right:
	lda r3
	clc
	adc #13
	cmp >_map_extent_x
	bcc .x_right_clamp_ok
	lda >_map_extent_x
	dec
.x_right_clamp_ok:
	sta r3
.x_not_right:

	; temp_y_2 = (temp_y + 13) >> 4
	lda r4
	clc
	adc #13
	lsr
	lsr
	lsr
	lsr
	cmp >_map_extent_tiles_y
	bcc .y2_clamp_ok
	lda >_map_extent_tiles_y
	dec
.y2_clamp_ok:
	sta r5				; r5 = temp_y_2

	; temp_x = temp_x_pixel >> 4
	lda r3
	lsr
	lsr
	lsr
	lsr
	sta r6				; r6 = temp_x

	; temp_y >>= 4
	lda r4
	lsr
	lsr
	lsr
	lsr
	sta r4				; r4 = temp_y

	; shiftcount = map_extent_tiles_x_shiftcount
	lda _map_extent_tiles_x_shiftcount
	tay
	lda r4
.shift_y1_x_loop:
	asl
	dey
	bne .shift_y1_x_loop
	sta r7				; r7 = shift_temp_y

	lda _map_extent_tiles_x_shiftcount
	tay
	lda r5
.shift_y2_x_loop:
	asl
	dey
	bne .shift_y2_x_loop
	sta r8				; r8 = shift_temp_y2

	; q = shift_temp_y + temp_x
	; q2 = shift_temp_y2 + temp_x
	clc
	adc r6
	tay				; Y = q2
	lda r7
	clc
	adc r6
	phx
	tax				; X = q

	sep #$20
	a8
	lda >_map_collision_buf,x
	sta r9
	tyx
	lda >_map_collision_buf,x
	sta r10
	rep #$20
	a16
	plx				; restore X = object offset

	; Check collision (< 128)
	lda r9
	and #$0080
	beq .coll_x_found
	lda r10
	and #$0080
	beq .coll_x_found
	bra .apply_x_pos

.coll_x_found:
	; TryNudgeCornerX logic
	lda r9
	and #$0080
	sta r11
	lda r10
	and #$0080
	cmp r11
	beq .x_nudge_failed

	; orig_y = o->pos.y.lh.h + 1
	lda !$000c,x
	inc
	sta r12

	lda r9
	and #$0080
	bne .x_nudge_bottom_blocked

	; Top corner blocked (c1 < 128)
	lda r12
	and #$000f
	eor #$000f
	inc
	cmp #5				; CORNER_NUDGE_THRESHOLD
	beq .x_nudge_down_ok
	bcc .x_nudge_down_ok
	bra .x_nudge_failed

.x_nudge_down_ok:
	lda !$000c,x
	inc
	sta !$000c,x
	clc
	adc !$002a,x			; + h
	sta !$002e,x			; b

	lda !$000c,x
	inc
	lsr
	lsr
	lsr
	lsr
	ldy _map_extent_tiles_x_shiftcount
.shift_nudge_down_loop:
	asl
	dey
	bne .shift_nudge_down_loop
	clc
	adc r6				; + temp_x
	phx
	tax
	sep #$20
	a8
	lda >_map_collision_buf,x
	rep #$20
	a16
	plx
	and #$0080
	beq .x_nudge_failed
	bra .apply_x_pos

.x_nudge_bottom_blocked:
	; Bottom corner blocked (c2 < 128)
	lda r12
	clc
	adc #13
	and #$000f
	inc
	cmp #5				; CORNER_NUDGE_THRESHOLD
	beq .x_nudge_up_ok
	bcc .x_nudge_up_ok
	bra .x_nudge_failed

.x_nudge_up_ok:
	lda !$000c,x
	dec
	sta !$000c,x
	clc
	adc !$002a,x			; + h
	sta !$002e,x			; b

	lda !$000c,x
	clc
	adc #14
	lsr
	lsr
	lsr
	lsr
	ldy _map_extent_tiles_x_shiftcount
.shift_nudge_up_loop:
	asl
	dey
	bne .shift_nudge_up_loop
	clc
	adc r6				; + temp_x
	phx
	tax
	sep #$20
	a8
	lda >_map_collision_buf,x
	rep #$20
	a16
	plx
	and #$0080
	beq .x_nudge_failed
	bra .apply_x_pos

.x_nudge_failed:
	; Stop X movement and flush position
	stz !$0012,x
	stz !$0014,x
	lda !$0014,x
	bmi .flush_x_left
	; moving right flush
	lda r6
	asl
	asl
	asl
	asl
	sec
	sbc #15
	sta r2
	stz r1
	bra .apply_x_pos
.flush_x_left:
	lda r6
	inc
	asl
	asl
	asl
	asl
	dec
	sta r2
	stz r1

.apply_x_pos:
	lda !$0012,x
	ora !$0014,x
	beq .skip_move_x
	lda r1
	sta !$0006,x
	lda r2
	sta !$0008,x
	clc
	adc !$0028,x
	sta !$002c,x

.skip_move_x:

	; Check has_y (delta_y != 0)
	lda !$0016,x
	ora !$0018,x
	beq .skip_move_y

	; temp_yl.a = pos.y.a + delta.y.a
	lda !$000a,x
	clc
	adc !$0016,x
	sta r1				; temp_yl low
	lda !$000c,x
	adc !$0018,x
	sta r2				; temp_yl high (pixel Y)

	; temp_x = pos.x.lh.h + 1
	lda !$0008,x
	inc
	bpl .x2_pixel_pos
	lda #$0000
	bra .x2_pixel_done
.x2_pixel_pos:
	cmp >_map_extent_x
	bcc .x2_pixel_done
	lda >_map_extent_x
	dec
.x2_pixel_done:
	sta r3				; r3 = temp_x (pixel)

	; temp_y_pixel = temp_yl.lh.h + 1
	lda r2
	inc
	bpl .y_pixel2_pos
	lda #$0000
	bra .y_pixel2_done
.y_pixel2_pos:
	cmp >_map_extent_y
	bcc .y_pixel2_done
	lda >_map_extent_y
	dec
.y_pixel2_done:
	sta r4				; r4 = temp_y_pixel

	; if (delta_y > 0)
	lda !$0018,x
	bmi .y_not_down
	bne .y_is_down
	lda !$0016,x
	beq .y_not_down
.y_is_down:
	lda r4
	clc
	adc #13
	cmp >_map_extent_y
	bcc .y_down_clamp_ok
	lda >_map_extent_y
	dec
.y_down_clamp_ok:
	sta r4
.y_not_down:

	; temp_y = temp_y_pixel >> 4
	lda r4
	lsr
	lsr
	lsr
	lsr
	sta r5				; r5 = temp_y (tile Y)

	; temp_x_2 = (temp_x + 13) >> 4
	lda r3
	clc
	adc #13
	lsr
	lsr
	lsr
	lsr
	sta r6				; r6 = temp_x_2 (right edge tile)

	; temp_x >>= 4
	lda r3
	lsr
	lsr
	lsr
	lsr
	sta r3				; r3 = temp_x (left edge tile)

	; shiftcount = map_extent_tiles_x_shiftcount
	lda _map_extent_tiles_x_shiftcount
	tay
	lda r5
.shift_y_y_loop:
	asl
	dey
	bne .shift_y_y_loop
	sta r7				; r7 = shift_temp_y

	; q = shift_temp_y + temp_x
	; q2 = shift_temp_y + temp_x_2
	clc
	adc r6
	tay				; Y = q2
	lda r7
	clc
	adc r3
	phx
	tax				; X = q

	sep #$20
	a8
	lda >_map_collision_buf,x
	sta r9
	tyx
	lda >_map_collision_buf,x
	sta r10
	rep #$20
	a16
	plx				; restore X = object offset

	; Check collision (< 128)
	lda r9
	and #$0080
	beq .coll_y_found
	lda r10
	and #$0080
	beq .coll_y_found
	bra .apply_y_pos

.coll_y_found:
	; TryNudgeCornerY logic
	lda r9
	and #$0080
	sta r11
	lda r10
	and #$0080
	cmp r11
	beq .y_nudge_failed

	; orig_x = o->pos.x.lh.h + 1
	lda !$0008,x
	inc
	sta r12

	lda r9
	and #$0080
	bne .y_nudge_right_blocked

	; Left corner blocked (c1 < 128)
	lda r12
	and #$000f
	eor #$000f
	inc
	cmp #5				; CORNER_NUDGE_THRESHOLD
	beq .y_nudge_right_ok
	bcc .y_nudge_right_ok
	bra .y_nudge_failed

.y_nudge_right_ok:
	lda !$0008,x
	inc
	sta !$0008,x
	clc
	adc !$0028,x			; + w
	sta !$002c,x			; r

	lda !$0008,x
	inc
	lsr
	lsr
	lsr
	lsr
	clc
	adc r7				; + shift_temp_y
	phx
	tax
	sep #$20
	a8
	lda >_map_collision_buf,x
	rep #$20
	a16
	plx
	and #$0080
	beq .y_nudge_failed
	bra .apply_y_pos

.y_nudge_right_blocked:
	; Right corner blocked (c2 < 128)
	lda r12
	clc
	adc #13
	and #$000f
	inc
	cmp #5				; CORNER_NUDGE_THRESHOLD
	beq .y_nudge_left_ok
	bcc .y_nudge_left_ok
	bra .y_nudge_failed

.y_nudge_left_ok:
	lda !$0008,x
	dec
	sta !$0008,x
	clc
	adc !$0028,x			; + w
	sta !$002c,x			; r

	lda !$0008,x
	clc
	adc #14
	lsr
	lsr
	lsr
	lsr
	clc
	adc r7				; + shift_temp_y
	phx
	tax
	sep #$20
	a8
	lda >_map_collision_buf,x
	rep #$20
	a16
	plx
	and #$0080
	beq .y_nudge_failed
	bra .apply_y_pos

.y_nudge_failed:
	; Stop Y movement and flush position
	stz !$0016,x
	stz !$0018,x
	lda !$0018,x
	bmi .flush_y_up
	; moving down flush
	lda r5
	asl
	asl
	asl
	asl
	sec
	sbc #15
	sta r2
	stz r1
	bra .apply_y_pos
.flush_y_up:
	lda r5
	inc
	asl
	asl
	asl
	asl
	dec
	sta r2
	stz r1

.apply_y_pos:
	lda !$0016,x
	ora !$0018,x
	beq .skip_move_y
	lda r1
	sta !$000a,x
	lda r2
	sta !$000c,x
	clc
	adc !$002a,x
	sta !$002e,x

.skip_move_y:

	plb
	plb
	lda #$0000
	rtl

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
