;vcprmin=10000
	section	"DONTMERGE_text.near.__irq_vblank.0","acrx"
	a16
	x16
	global	___irq_vblank
___irq_vblank:
	jml :+
	: ; Jump to fastROM
	rep	#$30
	
	pha
	phx
	phy

	phb
	phd

	lda #$0000
	tcd
	pha
	plb
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
	lda	_system_use_alternate_nmi
	bne	l4
	jsl	>_Nmi_Primary
	bra	l5
l4:
	jsl	>_Nmi_Alternate
l5:
	sep	#32
	a8
	lda	_snd_stream_enable
	a16
	rep	#32
	beq	l7
	jsl	>_SoundInterface_NmiAudioUpload
l7:
	jsl	>_SoundInterface_RunDeferredCommands

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
;vcprmin=10000
	section	"DONTMERGE_text.near.__irq_ext.0","acrx"
	a16
	x16
	global	___irq_ext
___irq_ext:
	jml :+
	: ; Jump to fastROM
	rep	#$30
	
	pha
	phx
	phy

	phb
	phd

	lda #$0000
	tcd
	pha
	plb
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
	jsl	>_Nmi_Cutscene
	sep	#32
	a8
	lda	_snd_stream_enable
	a16
	rep	#32
	beq	l11
	jsl	>_SoundInterface_NmiAudioUpload
l11:
	jsl	>_SoundInterface_RunDeferredCommands

	bit $4211

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
	global	_system_use_alternate_nmi
	zpage	_system_use_alternate_nmi
	global	_snd_stream_enable
	global	_SoundInterface_RunDeferredCommands
	global	_SoundInterface_NmiAudioUpload
	global	_Nmi_Primary
	global	_Nmi_Alternate
	global	_Nmi_Cutscene
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
