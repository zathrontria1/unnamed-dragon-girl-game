    section DONTMERGE_text.near.crashhandler.0
    a16
	x16
	global	_System_CrashHandler
_System_CrashHandler:
    ; set processor status bits to the correct state
    ; Interrupt flag should be set when coming in from a BRK or COP
    rep #$39 ; Clear MX, carry, decimal

    ; store the CPU registers in any possible way
    sta >_crashhandler_scratch

    ; disable NMI
    lda #$0000
    sta $804200 ; it's fine to hit the WRIO next to it
    sta $80420b ; disable DMA and HDMA too
    txa
    sta >_crashhandler_scratch+2
    tya
    sta >_crashhandler_scratch+4

    ; Stack items: processor state followed by PC
    pla
    sta >_crashhandler_scratch+6

    pla
    sta >_crashhandler_scratch+8

    tsc
    sta >_crashhandler_scratch+10

    ; Processor state
    tdc 
    sta >_crashhandler_scratch+12

    phb
    pla
    and #$00ff 
    sta >_crashhandler_scratch+14

    ; Pseudoregisters, integers
    rept 32, I
        lda >r0+(I * 2)
        sta >_crashhandler_scratch+16+(I * 2)
    endr

    ; Pseudoregisters, floats
    rept 8, I
        lda >btmp0+(I * 2)
        sta >_crashhandler_scratch+16+64+(I * 2)
    endr

    ; Should place things to prepare the screen for drawing debug text.

    stp ; Stop the CPU. It's done.

; stacksize=0+??
    global _crashhandler_scratch
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
