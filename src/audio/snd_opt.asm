; Optimized SPC700 upload routines extracted from snd.c
	section	"DONTMERGE_text.far._SoundInterface_UploadData.0","acrx"
	a16
	x16
	global	_SoundInterface_UploadData
	global	_SoundInterface_UploadData_2byte
	global	_SoundInterface_UploadData_2byte_StreamLoopBlock

; void SoundInterface_UploadData(uint8_t *data_ptr, uint16_t chunk_len)
_SoundInterface_UploadData:
	sta r0
	stx r0+2
	lda 4,s
	beq .upload_data_end
	sta r2
	ldy #0
	sep #$20
	a8
.upload_data_write:
	lda [r0],y
	sta 8513
	tya
	sta 8512
.upload_data_ack:
	cmp 8512
	bne .upload_data_ack
	iny
	cpy r2
	bcc .upload_data_write
	a16
	rep #$20
.upload_data_end:
	rtl

; void SoundInterface_UploadData_2byte(uint8_t *data_ptr, uint16_t chunk_len)
_SoundInterface_UploadData_2byte:
	a16
	x16
	sta r0
	stx r0+2
	stx r3+2
	lda 4,s
	beq .upload_2byte_end
	sta r2
	clc
	adc r0
	sta r3
	lda r4
	adc #0
	sta r4
	ldy #0
	sep #$20
	a8
.upload_2byte_write:
	lda [r0],y
	sta 8513
	lda [r3],y
	sta 8514
	tya
	sta 8512
.upload_2byte_ack:
	cmp 8512
	bne .upload_2byte_ack
	iny
	cpy r2
	bcc .upload_2byte_write
	a16
	rep #$20
.upload_2byte_end:
	rtl

; void SoundInterface_UploadData_2byte_StreamLoopBlock(uint8_t *data_ptr, uint16_t chunk_len)
_SoundInterface_UploadData_2byte_StreamLoopBlock:
	a16
	x16
	sta r0
	stx r0+2
	stx r3+2
	lda 4,s
	beq .upload_stream_end
	sta r2
	clc
	adc r0
	sta r3
	lda r4
	adc #0
	sta r4
	ldy #0
	sep #$20
	a8
.upload_stream_write:
	lda [r0],y
	sta 8513
	lda [r3],y
	cpy #27
	bne .upload_stream_no_flag
	ora #$03
.upload_stream_no_flag:
	sta 8514
	tya
	sta 8512
.upload_stream_ack:
	cmp 8512
	bne .upload_stream_ack
	iny
	cpy r2
	bcc .upload_stream_write
	a16
	rep #$20
.upload_stream_end:
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
