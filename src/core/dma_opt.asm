	section	"DONTMERGE_text.far._DmaSystem_CopyToWram.0","acrx"
	a16
	x16
	global	_DmaSystem_CopyToWram
_DmaSystem_CopyToWram:
	sta $4375

	lda r0
	sta $4372

	lda r2
	sta $2181

	lda #$8000
	sta $4370

	a8
	sep #$20

	lda r1
	sta $4374

	lda r3
	sta $2183

	lda #$80
	sta $420b

	a16
	rep #$20
	rtl

	section	"DONTMERGE_text.far._DmaSystem_CopyToWram_ShortRun.0","acrx"
	a16
	x16
	global	_DmaSystem_CopyToWram_ShortRun
_DmaSystem_CopyToWram_ShortRun:
	sta $4375

	lda r0
	sta $4372

	stx $2181

	a8
	sep #$20

	lda #$80
	sta $420b

	a16
	rep #$20
	rtl

	section	"DONTMERGE_text.far._DmaSystem_UploadOam.0","acrx"
	a16
	x16
	global	_DmaSystem_UploadOam
_DmaSystem_UploadOam:
	a8
	sep #$20

	ldx #0
	stx $2102

	stz $4300

	ldx #<_shadow_oam
	stx $4302
	lda #^_shadow_oam
	sta $4304

	ldx #544
	stx $4305

	lda #$04
	sta $4301

	lda #1
	sta $420b

	a16
	rep #$20
	rtl

	section	"DONTMERGE_text.far._DmaSystem_UploadCgram.0","acrx"
	a16
	x16
	global	_DmaSystem_UploadCgram
_DmaSystem_UploadCgram:
	a8
	sep #$20

	stz $2121
	stz $4300

	ldx #<_shadow_cgram
	stx $4302
	lda #^_shadow_cgram
	sta $4304

	ldx #512
	stx $4305

	lda #$22
	sta $4301

	lda #1
	sta $420b

	a16
	rep #$20
	rtl

	section	"DONTMERGE_text.far._DmaSystem_UploadCgram_Subset.0","acrx"
	a16
	x16
	global	_DmaSystem_UploadCgram_Subset
_DmaSystem_UploadCgram_Subset:
	a8
	sep #$20

	sta $2121
	stz $4300

	a16
	rep #$21

	asl
	adc #<_shadow_cgram
	sta $4302

	lda 4,s
	asl
	sta $4305

	a8
	sep #$20

	lda #^_shadow_cgram
	sta $4304

	lda #$22
	sta $4301

	lda #$01
	sta $420b

	a16
	rep #$20
	rtl

	section	"DONTMERGE_text.far._DmaSystem_UpdateStripTiles.0","acrx"
	a16
	x16
	global	_DmaSystem_UpdateStripTiles
_DmaSystem_UpdateStripTiles:
	a8
	sep #$20

	lda #$01
	sta $4300
	lda #$18
	sta $4301

	lda #$80
	sta $2115

	ldx _ani_bg_dest_water
	stx $2116

	ldx _ani_bg_addr_water
	stx $4302

	lda _ani_bg_addr_water+2
	sta $4304

	ldx #512
	stx $4305

	lda #$01
	sta $420b

	a16
	rep #$20
	rtl

	section	"DONTMERGE_text.far._DmaSystem_UpdateFrameTiles.0","acrx"
	a16
	x16
	global	_DmaSystem_UpdateFrameTiles
_DmaSystem_UpdateFrameTiles:
	a8
	sep #$20

	lda #$01
	sta $4300
	lda #$18
	sta $4301

	lda #$80
	sta $2115

	ldx _ani_bg_dest_tallbg
	stx $2116

	ldx _ani_bg_addr_tallbg
	stx $4302

	lda _ani_bg_addr_tallbg+2
	sta $4304

	ldx #2048
	stx $4305

	lda #$01
	sta $420b

	a16
	rep #$20
	rtl

	section	"DONTMERGE_text.far._DmaSystem_ProcessQueue.0","acrx"
	a16
	x16
	global	_DmaSystem_ProcessQueue
_DmaSystem_ProcessQueue:
	sep #$20
	a8
	lda _dma_queue_count
	beq .dma_queue_empty
	phd
	pea $4300
	pld
	sta $0b

	lda #$01
	sta $00
	lda #$18
	sta $01

	phy
	ldy #0
.loop_dma_queue:
	lda _dma_queue,y
	sta $2115
	ldx _dma_queue+2,y
	stx $02
	lda _dma_queue+4,y
	sta $04
	ldx _dma_queue+6,y
	stx $2116
	ldx _dma_queue+8,y
	stx $05
	lda #1
	sta $420b
	a16
	rep #$21
	tya
	adc #16
	tay
	sep #$20
	a8
	dec $0b
	bne .loop_dma_queue
	ply
	pld
.dma_queue_empty:
	rep #$20
	a16
	stz _dma_queue_count
	stz _dma_queue_length

	a8
	sep #$20
	lda _dma_filler_enable
	beq .process_queue_done

	lda #$09
	sta $4300

	lda #$80
	sta $2115

	a16
	rep #$20
	lda _dma_filler_dest
	sta $2116

	lda #<_dma_filler_val
	sta $4302

	lda _dma_filler_length
	sta $4305

	a8
	sep #$20
	lda #^_dma_filler_val
	sta $4304

	lda #$01
	sta $420b

	stz _dma_filler_enable
.process_queue_done:
	a16
	rep #$20
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
