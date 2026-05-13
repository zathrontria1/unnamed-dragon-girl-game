    zpage r0
    zpage r1
    zpage r2
    zpage r3
    global ___exit
    global ___start
    section start_text.startup
___start:
    clc
    xce
    jml :+ ; Jump to fastROM bank
    :
    rep #$ff
    sep #$24
    a8
    x16

    ; Reset three registers that must be reset ASAP
zero_byte: ; Refer to this + 1 to fetch a known source of zero
    stz $004200 ; Disable interrupts
    stz $00420c ; Disable HDMA

    lda #$8f
    sta $8f2100 ; Disable screen

    lda #$01
    sta $80420d ; Enable FastROM

    lda #$80
    pha
    plb ; Change to FastROM data bank
    
    rep #$30
    a16

    lda #___stackend
    tcs
    lda #r0
    and #$ff00
    tcd
    
    ; Near bank WRAM clear
    ; Write addresses and length of the clear area
    sep #$20
    a8
    ldx #<zero_byte+1
    stx r0
    lda #^zero_byte
    sta r1

    lda #$08
    sta r5
    
    ldx #<__NBS
    stx r2
    lda #^__NBS
    sta r3

    rep #$30 ; axy16
    a16
    x16

    lda #<__NBE
    sec
    sbc #<__NBS
    beq .clearfar
    sta r4
    
    jsl copy_dma
.clearfar:
    ; Repeat for the other banks.
    ; Far bank WRAM clear
    ; Write addresses and length of the clear area
    sep #$20
    a8
    ldx #<__FBS
    stx r2
    lda #^__FBS
    sta r3

    rep #$30 ; axy16
    a16
    x16

    lda #<__FBE
    sec
    sbc #<__FBS
    beq .clearhuge
    sta r4
    
    jsl copy_dma

    ; Huge bank WRAM clear
.clearhuge:
    sep #$20
    a8
    ldx #<__HBS
    stx r2
    lda #^__HBS
    sta r3

    rep #$30 ; axy16
    a16
    x16

    lda #<__HBE
    sec
    sbc #<__HBS
    beq .clear_done
    sta r4
    
    jsl copy_dma
.clear_done:
    ; Begin copying.
    ; C is the location of the source. S is destination.
    sep #$20
    a8
    ldx #<__NDC
    stx r0
    lda #^__NDC
    sta r1

    lda #$00
    sta r5
    
    ldx #<__NDS
    stx r2
    lda #^__NDS
    sta r3

    rep #$30 ; axy16
    a16
    x16

    lda #<__NDE
    sec
    sbc #<__NDS
    beq .copyfar
    sta r4
    
    jsl copy_dma
.copyfar:
    sep #$20
    a8
    ldx #<__FDC
    stx r0
    lda #^__FDC
    sta r1
    
    ldx #<__FDS
    stx r2
    lda #^__FDS
    sta r3

    rep #$30 ; axy16
    a16
    x16

    lda #<__FDE
    sec
    sbc #<__FDS
    beq .copyhuge
    sta r4
    
    jsl copy_dma

.copyhuge:
    sep #$20
    a8
    ldx #<__HDC
    stx r0
    lda #^__HDC
    sta r1
    
    ldx #<__HDS
    stx r2
    lda #^__HDS
    sta r3

    rep #$30 ; axy16
    a16
    x16

    lda #<__HDE
    sec
    sbc #<__HDS
    beq .copy_done
    sta r4
    
    jsl copy_dma

.copy_done:
    ; Clear the stack. Must be manually done without subroutine calls.
    sep #$20  ; 8-bit accumulator
    rep #$10  ; 16-bit index
    a8
    x16

    lda #^___stack
    sta $2183 
    ldx #<___stack
    stx $2181 ; WRAM address, bottom 16 bits

    ldx #___stacklen
    stx $4305

    ; Configure DMA to write to WMDATA
    lda #$08
    sta $4300

    lda #$80
    sta $4301

    ldx #<zero_byte+1
    stx $4302
    ; Set the bank byte of the source address too
    lda #^zero_byte
    sta $4304

    ; Start the DMA
    lda #$1
    sta $420b

    ; Clear the zero page
    stz $2183
    ldx #$0000
    stx $2181

    sta $4306
    
    sta $420b

    ; Get ready to call __main();
    rep #$30  ; 16-bit accumulator
    a16

    lda #$0000
    tax
    tay

    jsl ___main
___exit:
    jmp ___exit

copy_dma:
    ; r0/r1 source
    ; r2/r3 destination
    ; r4 length
    ; r5 fill/copy?
    php
    sep #$20  ; 8-bit accumulator
    rep #$10  ; 16-bit index
    a8
    x16

    lda r3
    sta $2183 
    ldx r2
    stx $2181 ; WRAM address, bottom 16 bits

    ldx r4
    stx $4305

    ; Configure DMA to write to WMDATA
    lda r5
    sta $4300

    lda #$80
    sta $4301

    ldx r0
    stx $4302
    ; Set the bank byte of the source address too
    lda r1
    sta $4304

    ; Start the DMA
    lda #$1
    sta $420b

    plp
    rtl

 section zpage
r0: reserve 2
r1: reserve 2
r2: reserve 2
r3: reserve 2
r4: reserve 2
r5: reserve 2
r6: reserve 2
r7: reserve 2
r8: reserve 2
r9: reserve 2
r10: reserve 2
r11: reserve 2
r12: reserve 2
r13: reserve 2
r14: reserve 2
r15: reserve 2
r16: reserve 2
r17: reserve 2
r18: reserve 2
r19: reserve 2
r20: reserve 2
r21: reserve 2
r22: reserve 2
r23: reserve 2
r24: reserve 2
r25: reserve 2
r26: reserve 2
r27: reserve 2
r28: reserve 2
r29: reserve 2
r30: reserve 2
r31: reserve 2

 global r0
 global r1
 global r2
 global r3
 global r4
 global r5
 global r6
 global r7
 global r8
 global r9
 global r10
 global r11
 global r12
 global r13
 global r14
 global r15
 global r16
 global r17
 global r18
 global r19
 global r20
 global r21
 global r22
 global r23
 global r24
 global r25
 global r26
 global r27
 global r28
 global r29
 global r30
 global r31






