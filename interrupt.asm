	section	"DONTMERGE_text.near.__irq_vblank.0","acrx"
	a16
	x16
	global	___irq_vblank
___irq_vblank:
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

	; pseudoregisters are not used in NMI

	jsl	>_interrupt_vblank_sub

	rep	#$30

	pld
	plb

	ply
	plx
	pla

	rti
; stacksize=0+??
	global	_interrupt_vblank_sub
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
