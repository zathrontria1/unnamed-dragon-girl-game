	section	"DONTMERGE_text.far._VwfEngine_PrintText_Render.0","acrx"
	a16
	x16
	global	_VwfEngine_PrintText_Render
_VwfEngine_PrintText_Render:
	a8
	sep #$20
	sta $4202

	lda r7
	sta r9

	a16
	rep #$21

	lda r6
	adc #16
	sta r8

	ldy #$0

	a8
	sep #$20

	rept 16, I
	a8

	lda [r4],y
	sta $4203

	a16
	rep #$20

	nop

	lda $4216

	a8
	sep #$20
	sta [r8],y
	xba
	ora [r6],y
	sta [r6],y

	iny

	endr

	a16
	x16
	rep #$30
	rtl

	section	"DONTMERGE_text.far._VwfEngine_PrintText_ResetTilemap.0","acrx"
	a16
	x16
	global	_VwfEngine_PrintText_ResetTilemap
_VwfEngine_PrintText_ResetTilemap:
	a16
	x16

	sta r0
	stx r1

	lda 4,s
	beq .end

	dec
	asl
	tay

	lda _vwf_tile_id_empty

.loop:

	sta [r0],y
	dey
	dey
	bpl .loop

.end:
	lda _vwf_tile_id_empty
	inc
	ora #$2000
	sta _vwf_tile_id

	lda _vwf_col_start
	asl
	sta r2

	lda _vwf_row_start
	asl
	asl
	asl
	asl
	asl
	asl
	clc
	adc r2
	clc
	adc _vwf_tilemap_ptr
	sta _vwf_tilemap_ptr
	bcc .tail_done
	inc _vwf_tilemap_ptr+2

.tail_done:
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
