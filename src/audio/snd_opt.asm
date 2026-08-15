; Optimized SPC700 NMI audio upload and deferred command processing routines

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
	sta _nmi_snd_scratch_ptr
	stx _nmi_snd_scratch_ptr+2
	stx _nmi_snd_scratch_ptr2+2
	lda 4,s
	beq .upload_2byte_end
	sta _nmi_snd_scratch_len
	clc
	adc _nmi_snd_scratch_ptr
	sta _nmi_snd_scratch_ptr2
	lda _nmi_snd_scratch_ptr+2
	adc #0
	sta _nmi_snd_scratch_ptr2+2
	ldy #0
	sep #$20
	a8
.upload_2byte_write:
	lda [_nmi_snd_scratch_ptr],y
	sta 8513
	lda [_nmi_snd_scratch_ptr2],y
	sta 8514
	tya
	sta 8512
.upload_2byte_ack:
	cmp 8512
	bne .upload_2byte_ack
	iny
	cpy _nmi_snd_scratch_len
	bcc .upload_2byte_write
	a16
	rep #$20
.upload_2byte_end:
	rtl

; void SoundInterface_UploadData_2byte_StreamLoopBlock(uint8_t *data_ptr, uint16_t chunk_len)
_SoundInterface_UploadData_2byte_StreamLoopBlock:
	sta _nmi_snd_scratch_ptr
	stx _nmi_snd_scratch_ptr+2
	stx _nmi_snd_scratch_ptr2+2
	lda 4,s
	beq .upload_stream_end
	sta _nmi_snd_scratch_len
	clc
	adc _nmi_snd_scratch_ptr
	sta _nmi_snd_scratch_ptr2
	lda _nmi_snd_scratch_ptr+2
	adc #0
	sta _nmi_snd_scratch_ptr2+2
	ldy #0
	sep #$20
	a8
.upload_stream_write:
	lda [_nmi_snd_scratch_ptr],y
	sta 8513
	lda [_nmi_snd_scratch_ptr2],y
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
	cpy _nmi_snd_scratch_len
	bcc .upload_stream_write
	a16
	rep #$20
.upload_stream_end:
	rtl

	section	"DONTMERGE_text.far.SoundInterface_AcknowledgeBusy.0","acrx"
	a16
	x16
	global	_SoundInterface_AcknowledgeBusy
_SoundInterface_AcknowledgeBusy:
	sep #$20
	a8
	cmp #0
	rep #$20
	a16
	bne .nmi_ack_busy_loop
	sep #$20
	a8
.nmi_ack_busy_wait1:
	lda $2140
	cmp _snd_current_command_counter
	bne .nmi_ack_busy_wait1
	rep #$20
	a16
	rtl

.nmi_ack_busy_loop:
	ldx #0
.nmi_ack_busy_wait2:
	sep #$20
	a8
	lda $2140
	cmp _snd_current_command_counter
	rep #$20
	a16
	beq .nmi_ack_busy_done
	inx
	cpx #256
	bcc .nmi_ack_busy_wait2

	sep #$20
	a8
	lda $2140
	sta _snd_current_command_counter
	rep #$20
	a16
.nmi_ack_busy_done:
	rtl

	section	"DONTMERGE_text.far.SoundInterface_AcknowledgeNop.0","acrx"
	a16
	x16
	global	_SoundInterface_AcknowledgeNop
_SoundInterface_AcknowledgeNop:
	sep #$20
	a8
	stz $2141
	rep #$20
	a16
	rtl

	section	"DONTMERGE_text.far.SoundInterface_IsHigherPriority.0","acrx"
	a16
	x16
	global	_SoundInterface_IsHigherPriority
_SoundInterface_IsHigherPriority:
	sep #$20
	a8
	sta _nmi_snd_scratch_temp
	cmp #37
	rep #$20
	a16
	bne .check_bounce
	lda #1
	rtl
.check_bounce:
	sep #$20
	a8
	lda _nmi_snd_scratch_temp
	cmp #42
	rep #$20
	a16
	beq .check_not_fire
	sep #$20
	a8
	lda _nmi_snd_scratch_temp
	cmp #41
	rep #$20
	a16
	bne .check_queued_protected
.check_not_fire:
	sep #$20
	a8
	lda _snd_defercmd_sfx_id
	cmp #37
	rep #$20
	a16
	beq .not_higher
	lda #1
	rtl
.check_queued_protected:
	sep #$20
	a8
	lda _snd_defercmd_sfx_id
	cmp #37
	beq .not_higher
	cmp #2
	beq .not_higher
	cmp #1
	beq .not_higher
	rep #$20
	a16
	lda #1
	rtl
.not_higher:
	rep #$30
	a16
	x16
	lda #0
	rtl

	section	"DONTMERGE_text.far.SoundInterface_PlaySfx_Internal.0","acrx"
	a16
	x16
	global	_SoundInterface_PlaySfx_Internal
_SoundInterface_PlaySfx_Internal:
	rep #$30
	a16
	x16
	sta _nmi_snd_scratch_temp
	lda #1
	jsl _SoundInterface_AcknowledgeBusy
	sep #$20
	a8
	lda _nmi_snd_scratch_temp
	sta $2142
	lda 4,s
	sta $2143
	lda #$04 ; SND_CMD_SFX_PLAY
	sta $2141
	inc _snd_current_command_counter
	rep #$30
	a16
	x16
	rtl

	section	"DONTMERGE_text.far.SoundInterface_PlaySfx_Ex_Internal.0","acrx"
	a16
	x16
	global	_SoundInterface_PlaySfx_Ex_Internal
_SoundInterface_PlaySfx_Ex_Internal:
	rep #$30
	a16
	x16
	sta _nmi_snd_scratch_temp
	lda #1
	jsl _SoundInterface_AcknowledgeBusy
	sep #$20
	a8
	lda _nmi_snd_scratch_temp
	sta $2142
	lda 8,s
	sta $2143
	lda #$05 ; SND_CMD_SFX_PLAY_EXTEND
	sta $2141
.play_sfx_ex_wait1:
	lda $2141
	cmp #$05
	bne .play_sfx_ex_wait1
	lda 4,s
	sta $2142
	lda 6,s
	sta $2143
	lda #$26 ; SND_CMD_SFX_PLAY_EXTEND_VOLDATA
	sta $2141
	inc _snd_current_command_counter
	rep #$30
	a16
	x16
	rtl

	section	"DONTMERGE_text.far.SoundInterface_StopSfx_Internal.0","acrx"
	a16
	x16
	global	_SoundInterface_StopSfx_Internal
_SoundInterface_StopSfx_Internal:
	rep #$30
	a16
	x16
	sta _nmi_snd_scratch_temp
	lda #1
	jsl _SoundInterface_AcknowledgeBusy
	sep #$20
	a8
	lda _nmi_snd_scratch_temp
	sta $2142
	lda #$06 ; SND_CMD_SFX_STOP
	sta $2141
	inc _snd_current_command_counter
	rep #$30
	a16
	x16
	rtl

	section	"DONTMERGE_text.far.SoundInterface_PlayStream.0","acrx"
	a16
	x16
	global	_SoundInterface_PlayStream
_SoundInterface_PlayStream:
	pei	(r16)
	pei	(r17)
	sta	r16
	stx	r16+2

	sep	#32
	a8
	lda	_snd_stream_enable
	beq	.start_new_stream
	a16
	rep	#32
	lda	_snd_stream_ptr_start
	cmp	r16
	bne	.restart_stream
	lda	2+_snd_stream_ptr_start
	cmp	r16+2
	bne	.restart_stream
	plx
	stx	r17
	plx
	stx	r16
	rtl

.start_new_stream:
	a16
	rep	#32
.restart_stream:
	stz	_snd_stream_current_block
	lda	r16
	sta	_snd_stream_ptr
	sta	_snd_stream_ptr_start
	lda	r16+2
	sta	2+_snd_stream_ptr
	sta	2+_snd_stream_ptr_start
	lda	8,s
	sta	_snd_stream_length
	sep	#32
	a8
	stz	_snd_defercmd_stream_stop_enable
	lda	10,s
	sta	_snd_stream_loop
	lda	#1
	sta	_snd_stream_starting
	sta	_snd_stream_enable
	a16
	plx
	stx	r17
	plx
	stx	r16
	rep	#32
	rtl

	section	"DONTMERGE_text.far.SoundInterface_PlayClip.0","acrx"
	a16
	x16
	global	_SoundInterface_PlayClip
_SoundInterface_PlayClip:
	sta	_nmi_snd_scratch_temp 
	sep	#32
	a8
	lda	_snd_settings_volume_voice
	a16
	rep	#32
	beq	l157
	lda	_nmi_snd_scratch_temp 
	asl
	asl
	asl
	sta	_nmi_snd_scratch_temp 
	tax
	lda	>6+_data_stream_table,x
	and	#255
	pha
	ldx _nmi_snd_scratch_temp 
	lda	>4+_data_stream_table,x
	pha
	
	lda	#<_data_stream_table
	clc
	adc	_nmi_snd_scratch_temp 
	sta	_nmi_snd_scratch_ptr
	lda	#^(_data_stream_table)
	adc	#0
	sta	_nmi_snd_scratch_ptr+2
	ldy	#2
	lda	[_nmi_snd_scratch_ptr],y
	tax
	lda	[_nmi_snd_scratch_ptr]
	jsl	>_SoundInterface_PlayStream
	ply
	ply
l157:
	rtl

	section	"DONTMERGE_text.far.SoundInterface_ResumeStream.0","acrx"
	a16
	x16
	global	_SoundInterface_ResumeStream
_SoundInterface_ResumeStream:
	sep	#32
	a8
	lda	#1
	sta	_snd_stream_enable
	a16
	rep	#32
	rtl

	section	"DONTMERGE_text.far.SoundInterface_PauseStream.0","acrx"
	a16
	x16
	global	_SoundInterface_PauseStream
_SoundInterface_PauseStream:
	sep	#32
	a8
	stz	_snd_stream_enable
	a16
	rep	#32
	rtl

	section	"DONTMERGE_text.far.SoundInterface_StopStream.0","acrx"
	a16
	x16
	global	_SoundInterface_StopStream
_SoundInterface_StopStream:
	sep	#32
	a8
	stz	_snd_stream_enable
	lda	#1
	sta	_snd_defercmd_stream_stop_enable
	a16
	rep	#32
	lda	_snd_stream_ptr_start
	sta	_snd_stream_ptr
	lda	2+_snd_stream_ptr_start
	sta	2+_snd_stream_ptr
	stz	_snd_stream_current_block
	rtl

	section	"DONTMERGE_text.far.SoundInterface_StopStream_Internal.0","acrx"
	a16
	x16
	global	_SoundInterface_StopStream_Internal
_SoundInterface_StopStream_Internal:
	lda	#1
	jsl	>_SoundInterface_AcknowledgeBusy
	sep	#$20
	a8
	lda	#$10 ; SND_CMD_STREAM_STOP
	sta	8513
	inc	_snd_current_command_counter
	rep	#$30
	a16
	x16
	rtl

	section	"DONTMERGE_text.far.SoundInterface_NmiAudioUpload.0","acrx"
	a16
	x16
	global	_SoundInterface_NmiAudioUpload
_SoundInterface_NmiAudioUpload:
	lda	#1
	jsl	>_SoundInterface_AcknowledgeBusy
	sep	#32
	a8
	lda	_snd_stream_starting
	sta	8515
	stz	_snd_stream_starting
	lda	_snd_stream_current_block
	sta	8514
	lda	#$11 ; SND_CMD_STREAM_UPLOAD
	sta	8513
	lda	8513
	cmp	#$11
	a16
	rep	#32
	beq	l348
l347:
	sep	#32
	a8
	lda	8513
	cmp	#$11
	a16
	rep	#32
	bne	l347
l348:
	lda	_snd_stream_current_block
	cmp	#3
	bne	l175
	pea	#36
	ldx	2+_snd_stream_ptr
	lda	_snd_stream_ptr
	jsl	>_SoundInterface_UploadData_2byte_StreamLoopBlock
	ply
	bra	l176
l175:
	pea	#36
	ldx	2+_snd_stream_ptr
	lda	_snd_stream_ptr
	jsl	>_SoundInterface_UploadData_2byte
	ply
l176:
	sep	#32
	a8
	lda	8512
	ina
	ina
	sta	8512
	inc	_snd_current_command_counter
	stz	8513
	a16
	rep	#32
	lda	_snd_stream_current_block
	ina
	and	#3
	sta	_snd_stream_current_block
	lda	_snd_stream_ptr
	clc
	adc	#72
	sta	_snd_stream_ptr
	ldx	2+_snd_stream_ptr_start
	lda	_snd_stream_ptr_start
	clc
	adc	_snd_stream_length
	pha
	cpx	2+_snd_stream_ptr
	bcc	l371
	bne	l370
	cmp	_snd_stream_ptr
	bcc	l371
	bne	l370
l371:
	pla
	lda	_snd_stream_ptr_start
	sta	_snd_stream_ptr
	lda	2+_snd_stream_ptr_start
	sta	2+_snd_stream_ptr
	sep	#32
	a8
	lda	_snd_stream_loop
	a16
	rep	#32
	bne	l183
	ldx	#^(_data_snd_stream_silence)
	lda	#<_data_snd_stream_silence
	cpx	2+_snd_stream_ptr_start
	bne	l372
	cmp	_snd_stream_ptr_start
	beq	l182
l372:
	lda	#0
	jml	>_SoundInterface_PlayClip
l182:
	jsl	>_SoundInterface_StopStream
l183:
	rtl
l370:
	pla
	bra	l183

	section	"DONTMERGE_text.far.SoundInterface_RunDeferredCommands.0","acrx"
	a16
	x16
	global	_SoundInterface_RunDeferredCommands
_SoundInterface_RunDeferredCommands:
	rep #$30
	a16
	x16
	sep #$20
	a8
	lda _snd_defercmd_sfx_enable
	rep #$20
	a16
	beq .check_stop_sfx
	sep #$20
	a8
	lda _snd_defercmd_sfx_use_extended_format
	rep #$20
	a16
	beq .play_normal_sfx
	lda _snd_defercmd_sfx_pitch
	and #$00ff
	pha
	lda _snd_defercmd_sfx_vol_r
	and #$00ff
	pha
	lda _snd_defercmd_sfx_vol
	and #$00ff
	pha
	lda _snd_defercmd_sfx_id
	and #$00ff
	jsl _SoundInterface_PlaySfx_Ex_Internal
	sep #$20
	a8
	stz _snd_defercmd_sfx_use_extended_format
	rep #$20
	a16
	ply
	ply
	ply
	bra .reset_sfx_flags

.play_normal_sfx:
	lda _snd_defercmd_sfx_vol
	and #$00ff
	pha
	lda _snd_defercmd_sfx_id
	and #$00ff
	jsl _SoundInterface_PlaySfx_Internal
	ply

.reset_sfx_flags:
	sep #$20
	a8
	stz _snd_defercmd_sfx_enable
	stz _snd_defercmd_sfx_id
	rep #$20
	a16

.check_stop_sfx:
	sep #$20
	a8
	lda _snd_defercmd_sfx_stop_enable
	rep #$20
	a16
	beq .check_stop_stream
	lda _snd_defercmd_sfx_stop_sfx_id
	and #$00ff
	jsl _SoundInterface_StopSfx_Internal
	sep #$20
	a8
	stz _snd_defercmd_sfx_stop_enable
	stz _snd_defercmd_sfx_stop_sfx_id
	rep #$20
	a16

.check_stop_stream:
	sep #$20
	a8
	lda _snd_defercmd_stream_stop_enable
	rep #$20
	a16
	beq .deferred_done
	jsl _SoundInterface_StopStream_Internal
	sep #$20
	a8
	stz _snd_defercmd_stream_stop_enable
	rep #$20
	a16

.deferred_done:
	rep #$30
	a16
	x16
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
