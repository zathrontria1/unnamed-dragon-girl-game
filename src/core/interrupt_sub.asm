	section	"DONTMERGE_text.far._Nmi_Primary.0","acrx"
	a16
	x16
	global	_Nmi_Primary
_Nmi_Primary:
	sep #$20
	a8

	lda _shadow_brightness+1
	ora _shadow_fblank_enable
	sta $2100

	lda $213e
	sta _shadow_stat77

	lda _shadow_hdmaen
	sta $420c

	rep #$20
	a16
	lda _hdma_gradient_ptr
	sta $4322

	inc _system_nmis_counted

	lda _system_in_vblank
	and #$00ff
	beq .check_fadein
	lda _system_nmis_counted
	cmp #2
	bcs .vblank_ready

.check_fadein:
	lda _system_current_routine
	cmp #65500
	beq .vblank_ready
	jmp .vblank_not_ready

.vblank_ready:
	stz _system_nmis_counted
	sep #$20
	a8
	stz _system_in_vblank
	rep #$20
	a16

	lda _hdma_scroll_ptr
	sta $4332
	lda _hdma_coldata_ptr
	sta $4342

	jsl _DmaSystem_NmiDmaTransfer

	lda _bg_scroll_y+2
	dec
	sta _bg_scroll_y_mod+2

	sep #$20
	a8
	lda _ui_in_bg2
	bne .scroll_bg1

	lda _bg_scroll_x+2
	sta $210f
	lda _bg_scroll_x+3
	sta $210f
	lda _bg_scroll_y_mod+2
	sta $2110
	lda _bg_scroll_y_mod+3
	sta $2110
	bra .scroll_done

.scroll_bg1:
	lda _bg_scroll_x+2
	sta $210d
	lda _bg_scroll_x+3
	sta $210d
	lda _bg_scroll_y_mod+2
	sta $210e
	lda _bg_scroll_y_mod+3
	sta $210e

.scroll_done:
	lda _shadow_mosaic
	sta $2106
	lda _shadow_cgwsub
	sta $2130
	lda _shadow_cgadsub
	sta $2131
	lda _shadow_coldata_r
	sta $2132
	lda _shadow_coldata_g
	sta $2132
	lda _shadow_coldata_b
	sta $2132

	rep #$20
	a16
	jsl _System_UpdateFrameCounters
	rtl

.vblank_not_ready:
	pea 4
	lda #$0004
	jsl _DmaSystem_UploadCgram_Subset
	pea 6
	lda #$0039
	jsl _DmaSystem_UploadCgram_Subset
	ply
	ply

	lda _system_suppress_odd_transfers
	and #$00ff
	bne .suppress_transfers

	lda _ani_bg_water_dma_ready
	and #$00ff
	beq .check_tallbg
	jsl _DmaSystem_UpdateStripTiles
	sep #$20
	a8
	stz _ani_bg_water_dma_ready
	rep #$20
	a16

.check_tallbg:
	lda _ani_bg_tallbg_dma_ready
	and #$00ff
	beq .check_lag
	jsl _DmaSystem_UpdateFrameTiles
	sep #$20
	a8
	stz _ani_bg_tallbg_dma_ready
	rep #$20
	a16
	bra .check_lag

.suppress_transfers:
	sep #$20
	a8
	stz _ani_bg_water_dma_ready
	stz _ani_bg_tallbg_dma_ready
	rep #$20
	a16

.check_lag:
	lda _system_nmis_counted
	cmp #2
	bcc .nmi_done
	lda _system_dont_count_lag
	and #$00ff
	bne .nmi_done
	inc _system_frames_lag

.nmi_done:
	rtl

	section	"DONTMERGE_text.far._Nmi_Alternate.0","acrx"
	a16
	x16
	global	_Nmi_Alternate
_Nmi_Alternate:
	sep #$20
	a8

	lda _shadow_brightness+1
	ora _shadow_fblank_enable
	sta $2100

	stz _system_in_vblank
	rep #$20
	a16
	stz _system_nmis_counted

	lda _shadow_brightness
	bne .check_upper_clamp
	lda _shadow_brightness_change
	bpl .check_upper_clamp
	stz _shadow_brightness
	stz _shadow_brightness_change
	bra .apply_change

.check_upper_clamp:
	lda _shadow_brightness
	cmp #$0f00
	bcc .apply_change
	lda _shadow_brightness_change
	bmi .apply_change
	lda #$0f00
	sta _shadow_brightness
	stz _shadow_brightness_change
	bra .mosaic

.apply_change:
	lda _shadow_brightness_change
	beq .mosaic
	clc
	adc _shadow_brightness
	sta _shadow_brightness

.mosaic:
	jsl _Gfx_ProcessMosaic

	sep #$20
	a8
	lda _shadow_mosaic
	sta $2106
	lda _shadow_cgwsub
	sta $2130
	lda _shadow_cgadsub
	sta $2131
	lda _shadow_coldata_r
	sta $2132
	lda _shadow_coldata_g
	sta $2132
	lda _shadow_coldata_b
	sta $2132

	rep #$20
	a16
	rtl

	section	"DONTMERGE_text.far._Nmi_Cutscene.0","acrx"
	a16
	x16
	global	_Nmi_Cutscene
_Nmi_Cutscene:
	sep #$20
	a8

	lda _shadow_brightness+1
	ora #$80
	sta $2100

	lda $213e
	sta _shadow_stat77

	rep #$20
	a16
	inc _system_nmis_counted

	lda _system_in_vblank
	and #$00ff
	beq .cutscene_check_lag

	sep #$20
	a8
	stz _system_in_vblank
	rep #$20
	a16

	jsl _DmaSystem_NmiDmaTransfer

	sep #$20
	a8
	lda _shadow_mosaic
	sta $2106
	rep #$20
	a16

	lda _system_nmis_counted
	cmp #2
	bcc .cutscene_check_lag

	jsl _System_UpdateFrameCounters
	stz _system_nmis_counted

.cutscene_check_lag:
	lda _system_nmis_counted
	cmp #2
	bcc .cutscene_restore_display
	lda _system_dont_count_lag
	and #$00ff
	bne .cutscene_restore_display
	inc _system_frames_lag

.cutscene_restore_display:
	sep #$20
	a8
	lda _shadow_brightness+1
	ora _shadow_fblank_enable
	sta $2100

	rep #$20
	a16
	rtl
