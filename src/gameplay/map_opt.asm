	section	"DONTMERGE_text.far._MapSystem_BuildCollisionTable.0","acrx"
	a16
	x16
	global	_MapSystem_BuildCollisionTable
_MapSystem_BuildCollisionTable:
	; r0 = 24-bit pointer to (map_current + 2)
	lda	_map_current
	clc
	adc	#2
	sta	r0
	lda	_map_current+2
	sta	r1			; bank in r2

	; r3 = 24-bit pointer to map_lut_col
	lda	_map_lut_col
	sta	r3
	lda	_map_lut_col+2
	sta	r4

	; Calculate screens_x and screens_y
	lda	_map_extent_tiles_x
	lsr
	lsr
	lsr
	lsr
	sta	r6			; screens_x

	lda	_map_extent_tiles_y
	lsr
	lsr
	lsr
	lsr
	sta	r7			; screens_y

	lda	_map_extent_tiles_x_shiftcount
	clc
	adc	#4
	sta	r8			; shiftcount_total

	stz	r9			; sy = 0

.sy_loop:
	lda	r9
	cmp	r7
	bge	.done

	; sy_offset = sy << shiftcount_total
	lda	r9
	ldx	r8
.shift_loop:
	asl
	dex
	bne	.shift_loop
	sta	r12			; sy_offset

	stz	r10			; sx = 0

.sx_loop:
	lda	r10
	cmp	r6
	bge	.next_sy

	; Pointer r0 = (map_current + 2) + sy_offset + (sx << 8)
	lda	_map_current
	clc
	adc	#2
	clc
	adc	r12
	sta	r13
	lda	r10
	xba
	and	#$ff00
	clc
	adc	r13
	sta	r0			; r0 lower 16 bits updated for screen (sx, sy)

	; dst_base_idx = sy_offset + (sx << 4)
	lda	r10
	asl
	asl
	asl
	asl
	clc
	adc	r12
	sta	r14			; r14 = dst_base_idx

	stz	r11			; ty = 0

.ty_loop:
	lda	r11
	cmp	#16
	bge	.next_sx

	; Destination WRAM index X in _map_collision_buf
	ldx	r14

	; Source byte index Y in current screen = (ty << 4)
	lda	r11
	asl
	asl
	asl
	asl
	tay

	; Pre-zero accumulator C (B=0, A=0) so B is 0x00 when switching to a8
	lda	#$0000
	a8
	sep	#$20

	; Copy 16 tiles for row ty of screen (sx, sy) using vasm rept/endr
	rept 16
	lda	[r0],y
	iny
	phy
	tay
	lda	[r3],y
	ply
	sta	>_map_collision_buf,x
	inx
	endr

	; Back to 16-bit accumulator and clear carry
	a16
	rep	#$21

	lda	r14
	adc	_map_extent_tiles_x
	sta	r14

	inc	r11
	bra	.ty_loop

.next_sx:
	inc	r10
	bra	.sx_loop

.next_sy:
	inc	r9
	bra	.sy_loop

.done:
	a16
	x16
	rep	#$30
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
