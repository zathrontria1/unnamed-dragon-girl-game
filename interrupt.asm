	section	"DONTMERGE_text.near.__irq_vblank.0","acrx"
	a16
	x16
	global	___irq_vblank
___irq_vblank:
;startinline
	jml >.fast_nmi
.fast_nmi:

;endinline
	rep	#$30
	pha
	phx
	phy

	phb
	phd

	lda #$0000
	tcd
	pha
	plb ; Cannot phk, Program Bank is C0. Reuse the value in A.
	plb

	pei	(r28)
	pei	(r29)
	pei	(r30)
	pei	(r31)
	pei	(r0)
	pei	(r1)
	pei	(r2)
	pei	(r3)
	pei	(r4)
	pei	(r5)
	pei	(r6)
	pei	(r7)
	pei	(r8)
	pei	(r9)
	pei	(r10)
	pei	(r11)
	pei	(r12)
	pei	(r13)
	pei	(r14)
	pei	(r15)

; volatile barrier
	sep	#32
	a8
	lda	_shadow_inidisp
	sta	8448
; volatile barrier
	lda	8510
	sta	_shadow_stat77
	a16
	rep	#32
	lda	_system_current_routine
	cmp	#10
	bne	l8
; volatile barrier
	sep	#32
	a8
	lda	#70
	sta	16908
	a16
	rep	#32
	bra	l14
l8:
	lda	_system_current_routine
	cmp	#100
	beq	l14
	lda	_system_current_routine
	sec
	sbc	#65500
	cmp	#1
	bcc	l14
	beq	l14
; volatile barrier
	sep	#32
	a8
	lda	#2
	sta	16908
	a16
	rep	#32
l14:
	inc	_system_nmis_counted
	lda	_system_in_vblank
	beq	l16
	lda	_system_nmis_counted
	cmp	#2
	bcs	l15
	lda	_system_current_routine
	cmp	#65500
	beq	l15
	lda	_system_current_routine
	cmp	#65501
	bne	l16
l15:
	stz	_system_in_vblank
	stz	_system_nmis_counted
	jsl	>_dma_copy_oam
	jsl	>_dma_copy_palette
	jsl	>_dma_queue_process
	lda	2+_bg_scroll_y
	dea
	sta	2+_bg_scroll_y_mod
	lda	_system_ui_in_bg2
	bne	l21
; volatile barrier
	sep	#32
	a8
	lda	2+_bg_scroll_x
	sta	8463
; volatile barrier
	lda	3+_bg_scroll_x
	sta	8463
; volatile barrier
	lda	2+_bg_scroll_y_mod
	sta	8464
; volatile barrier
	lda	3+_bg_scroll_y_mod
	sta	8464
	a16
	rep	#32
	bra	l22
l21:
; volatile barrier
	sep	#32
	a8
	lda	2+_bg_scroll_x
	sta	8461
; volatile barrier
	lda	3+_bg_scroll_x
	sta	8461
; volatile barrier
	lda	2+_bg_scroll_y_mod
	sta	8462
; volatile barrier
	lda	3+_bg_scroll_y_mod
	sta	8462
	a16
	rep	#32
l22:
	inc	_system_frames_elapsed
	bne	l33
	inc	2+_system_frames_elapsed
l33:
	bra	l29
l16:
	lda	_system_current_routine
	cmp	#100
	beq	l29
	pea	#13
	lda	#66
	jsl	>_dma_copy_palette_subset
	ply
	lda	_ani_bg_water_dma_ready
	beq	l27
	jsl	>_dma_copy_bg_water_anim
	stz	_ani_bg_water_dma_ready
l27:
	lda	_ani_bg_tallbg_dma_ready
	beq	l29
	jsl	>_dma_copy_bg_64height_anim
	stz	_ani_bg_tallbg_dma_ready
l29:
	rep	#$30
	plx
	stx	r15
	plx
	stx	r14
	plx
	stx	r13
	plx
	stx	r12
	plx
	stx	r11
	plx
	stx	r10
	plx
	stx	r9
	plx
	stx	r8
	plx
	stx	r7
	plx
	stx	r6
	plx
	stx	r5
	plx
	stx	r4
	plx
	stx	r3
	plx
	stx	r2
	plx
	stx	r1
	plx
	stx	r0
	pla
	sta	r31
	pla
	sta	r30
	pla
	sta	r29
	pla
	sta	r28

	pld
	plb

	ply
	plx
	pla
	rti
; stacksize=0+??
	global	_system_in_vblank
	zpage	_system_in_vblank
	global	_system_current_routine
	zpage	_system_current_routine
	global	_system_frames_elapsed
	zpage	_system_frames_elapsed
	global	_shadow_stat77
	zpage	_shadow_stat77
	global	_shadow_inidisp
	zpage	_shadow_inidisp
	global	_system_nmis_counted
	zpage	_system_nmis_counted
	global	_bg_scroll_x
	zpage	_bg_scroll_x
	global	_bg_scroll_y
	zpage	_bg_scroll_y
	global	_bg_scroll_y_mod
	zpage	_bg_scroll_y_mod
	global	_ani_bg_water_dma_ready
	zpage	_ani_bg_water_dma_ready
	global	_ani_bg_tallbg_dma_ready
	zpage	_ani_bg_tallbg_dma_ready
	global	_system_ui_in_bg2
	global	_dma_copy_oam
	global	_dma_copy_palette
	global	_dma_copy_palette_subset
	global	_dma_queue_process
	global	_dma_copy_bg_water_anim
	global	_dma_copy_bg_64height_anim
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
