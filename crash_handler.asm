    section DONTMERGE_text.near.crashhandler.0
    a16
	x16
	global	_System_CrashHandler
_System_CrashHandler:
    ; set processor status bits to the correct state
    ; Interrupt flag should be set when coming in from a BRK or COP
    rep #$39 ; Clear MX, carry, decimal

    ; store the CPU registers in any possible way
    sta >_crashhandler_a

    ; disable NMI
    lda #$0000
    sta $804200 ; it's fine to hit the WRIO next to it
    sta $80420b ; disable DMA and HDMA too
    txa
    sta >_crashhandler_x
    tya
    sta >_crashhandler_y

    ; Stack items: processor state followed by PC
    sep #$20
    a8
    pla
    sta >_crashhandler_flags

    pla
    sta >_crashhandler_pc

    lda #$00
    sta >_crashhandler_pc+3

    phb
    pla
    sta >_crashhandler_databank

    rep #$20
    a16
    pla
    sta >_crashhandler_pc+1

    tsc
    sta >_crashhandler_sp

    ; Processor state not in stack
    tdc 
    sta >_crashhandler_directpage

    lda #$0000 ; reset DP
    tcd

    ldx #$003e

    ; Pseudoregisters, integers
    .loop_save_pseudoregs:
    lda r0,x
    sta >_crashhandler_regs,x
    dex
    dex
    bpl .loop_save_pseudoregs

    ;rept 32, I
    ;    lda >r0+(I * 2)
    ;    sta >_crashhandler_regs+(I * 2)
    ;endr

    ; Pseudoregisters, floats

    ldx #$000e

    .loop_save_floats:
    lda btmp0,x
    sta >_crashhandler_regs_float,x
    dex
    dex
    bpl .loop_save_floats

    ;rept 8, I
    ;    lda >btmp0+(I * 2)
    ;    sta >_crashhandler_regs_float+(I * 2)
    ;endr

    pea #$8080
    plb
    plb ; reset DBR
    
    ; Stack
    tsx
    inx
    txy
    ldx #$0000
    cpy #$2000 ; is it out of range?
    bcs .stack_invalid
        ; stack valid
        .loop_save_stack:
            lda !$0000,y
            ; check if it's at the last valid byte
            cpy #$1fff
            bcc .no_open_bus
                and #$00ff ; discard high byte
            .no_open_bus:
            sta >_crashhandler_stack,x
            inx
            inx
            iny
            iny
            cpy #$2000
            bcs .loop_save_invalid_stack_start
            cpx #$0020
            bcc .loop_save_stack
        bra .stack_done
    .stack_invalid:
        ; stack invalid
        .loop_save_invalid_stack_start:
        cpx #$0020
        bcs .stack_done
        lda #$0000
        .loop_save_invalid_stack:
            sta >_crashhandler_stack,x
            inx
            inx
            cpx #$0020
            bcc .loop_save_invalid_stack
    .stack_done:

    ;rept 16, I
    ;    lda 1+(I * 2),s
    ;    sta >_crashhandler_stack+(I * 2)
    ;endr
    
    lda #$1fff
    tcs

    ; Call the full handler after processor state is preserved
    jsl _System_CrashHandler_Followup
    
    stp ; Stop the CPU. It's done.

; stacksize=0+??
    global _crashhandler_a
    global _crashhandler_x
    global _crashhandler_y
    global _crashhandler_flags
    global _crashhandler_pc
    global _crashhandler_sp
    global _crashhandler_directpage
    global _crashhandler_databank

    global _crashhandler_regs
    global _crashhandler_regs_float

    global _crashhandler_stack

    global _System_CrashHandler_Followup
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
