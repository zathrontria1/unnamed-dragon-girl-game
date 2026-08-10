    zpage r0
    zpage r1
    zpage r2
    zpage r3
    zpage r4
    zpage r5
    zpage r6
    global ___exit
    global ___start
    section start_text.startup

zero_byte:
    byte $00

___start:
    clc
    xce
    jml :+ ; Jump to fastROM bank
    :
    rep #$ff
    sep #$24
    a8
    x16

    ; Reset two hardware control registers ASAP
    stz $004200 ; Disable interrupts (NMI/VBLANK/H-V timer)
    stz $00420c ; Disable HDMA

    lda #$8f
    sta $8f2100 ; Disable screen (forced blank)

    lda #$01
    sta $80420d ; Enable FastROM

    lda #<__DBR_init
    pha
    plb ; Change Data Bank to FastROM bank (__DBR_init)
    
    rep #$30
    a16
    x16

    lda #___stackend
    tcs
    lda #r0
    and #$ff00
    tcd
    
    ; Clear Near BSS
    sep #$20
    a8
    ldx #<__NBS
    stx r2
    lda #^__NBS
    sta r3
    ldx #<__NBE
    stx r0
    lda #^__NBE
    sta r1
    jsr clear_section_dma

    ; Clear Far BSS
    sep #$20
    a8
    ldx #<__FBS
    stx r2
    lda #^__FBS
    sta r3
    ldx #<__FBE
    stx r0
    lda #^__FBE
    sta r1
    jsr clear_section_dma

    ; Clear Huge BSS
    sep #$20
    a8
    ldx #<__HBS
    stx r2
    lda #^__HBS
    sta r3
    ldx #<__HBE
    stx r0
    lda #^__HBE
    sta r1
    jsr clear_section_dma

    ; Copy Near Data
    sep #$20
    a8
    ldx #<__NDC
    stx r0
    lda #^__NDC
    sta r1
    ldx #<__NDS
    stx r2
    lda #^__NDS
    sta r3
    ldx #<__NDE
    stx r4
    lda #^__NDE
    sta r5
    jsr copy_section_dma

    ; Copy Far Data
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
    ldx #<__FDE
    stx r4
    lda #^__FDE
    sta r5
    jsr copy_section_dma

    ; Copy Huge Data
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
    ldx #<__HDE
    stx r4
    lda #^__HDE
    sta r5
    jsr copy_section_dma

    ; Clear the stack area
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

    ; Configure DMA channel 0 to write zero to WMDATA
    lda #$08
    sta $4300
    lda #$80
    sta $4301

    ldx #<zero_byte
    stx $4302
    lda #^zero_byte
    sta $4304

    ; Start DMA for stack clear
    lda #$01
    sta $420b

    ; Clear Direct Page / Zero Page (256 bytes at 0x0000)
    stz $2183
    ldx #$0000
    stx $2181

    ldx #$0100
    stx $4305
    
    lda #$01
    sta $420b

    ; Get ready to call __main();
    rep #$30  ; 16-bit accumulator and index
    a16
    x16

    lda #$0000
    tax
    tay

    jsl ___main
___exit:
    jmp ___exit

clear_section_dma:
    ; r2/r3 = 24-bit WRAM start address
    ; r0/r1 = 24-bit WRAM end address
    php
    rep #$30
    a16
    x16

    ; Calculate 24-bit length = end (r0/r1) - start (r2/r3)
    lda r0
    sec
    sbc r2
    sta r4      ; r4 = low 16 bits of length
    sep #$20
    a8
    lda r1
    sbc r3
    sta r5      ; r5 = bank byte of length

    ; Check if length is 0 (r5 == 0 && r4 == 0)
    bne .clear_start
    rep #$30
    a16
    x16
    lda r4
    beq .clear_exit

.clear_start:
    ; Set WRAM destination address ($2181-$2183)
    sep #$20
    a8
    lda r3
    sta $2183 
    ldx r2
    stx $2181

    ; Configure DMA channel 0 to write zero to WMDATA ($2180)
    lda #$08
    sta $4300   ; Fixed source address, write 1 byte to 1 register ($2180)
    lda #$80
    sta $4301   ; WMDATA ($2180)
    ldx #<zero_byte
    stx $4302
    lda #^zero_byte
    sta $4304

.clear_loop:
    sep #$20
    a8
    lda r5
    beq .clear_last_chunk

    ; r5 > 0: Transfer 64KB (0x0000 in $4305)
    rep #$30
    a16
    x16
    ldy #$0000
    sty $4305
    sep #$20
    a8
    lda #$01
    sta $420b   ; Start DMA channel 0
    dec r5      ; Decrement high bank count
    bra .clear_loop

.clear_last_chunk:
    rep #$30
    a16
    x16
    lda r4
    beq .clear_exit

    sta $4305   ; Transfer remaining r4 bytes
    sep #$20
    a8
    lda #$01
    sta $420b   ; Start DMA channel 0

.clear_exit:
    plp
    rep #$30
    a16
    x16
    rts

copy_section_dma:
    ; r0/r1 = 24-bit ROM source start
    ; r2/r3 = 24-bit WRAM dest start
    ; r4/r5 = 24-bit WRAM dest end
    php
    rep #$30
    a16
    x16

    ; Calculate 24-bit length = dest_end (r4/r5) - dest_start (r2/r3)
    lda r4
    sec
    sbc r2
    sta r4      ; r4 = remaining length low 16 bits
    sep #$20
    a8
    lda r5
    sbc r3
    sta r5      ; r5 = remaining length high bank byte

    ; Check if length is 0 (r5 == 0 && r4 == 0)
    bne .copy_start
    rep #$30
    a16
    x16
    lda r4
    beq .copy_exit

.copy_start:
    ; Set WRAM destination address ($2181-$2183)
    sep #$20
    a8
    lda r3
    sta $2183
    ldx r2
    stx $2181

    ; Configure DMA channel 0 mode (incrementing source, write 1 byte to WMDATA $2180)
    lda #$00
    sta $4300
    lda #$80
    sta $4301

.copy_loop:
    ; Check if remaining length is 0 (r5 == 0 && r4 == 0)
    sep #$20
    a8
    lda r5
    bne .copy_do_chunk
    rep #$30
    a16
    x16
    lda r4
    beq .copy_exit

.copy_do_chunk:
    ; Calculate bytes left in current 64KB ROM bank:
    ; avail = 0x10000 - r0
    rep #$30
    a16
    x16
    lda #$0000
    sec
    sbc r0      ; A = bytes left in current ROM bank (0x0000 means 65536)

    ; Determine chunk_size (store in r6)
    sep #$20
    a8
    lda r5      ; remaining high bank byte
    beq .copy_check_small

    ; remaining length >= 64KB (r5 > 0) -> use avail (in A)
    rep #$30
    a16
    x16
    sta r6      ; r6 = chunk_size (avail)
    bra .copy_execute

.copy_check_small:
    ; remaining length < 64KB (r5 == 0, length in r4)
    rep #$30
    a16
    x16
    cmp #$0000  ; Is avail == 64KB (0x0000)?
    beq .copy_use_r4
    cmp r4      ; compare avail with r4
    bcc .copy_use_avail ; if avail < r4, use avail
.copy_use_r4:
    lda r4
    sta r6      ; r6 = r4
    bra .copy_execute
.copy_use_avail:
    sta r6      ; r6 = avail

.copy_execute:
    ; r6 contains chunk_size to transfer
    ; Set DMA source
    ldx r0
    stx $4302
    sep #$20
    a8
    lda r1
    sta $4304

    ; Set DMA length
    rep #$30
    a16
    x16
    ldx r6
    stx $4305

    ; Start DMA channel 0
    sep #$20
    a8
    lda #$01
    sta $420b

    ; Advance ROM source address r0/r1 by r6
    rep #$30
    a16
    x16
    lda r0
    clc
    adc r6      ; r0 += r6
    sta r0
    bcc .copy_sub_len
    ; r0 wrapped 64KB boundary, advance r1 bank byte
    sep #$20
    a8
    inc r1

.copy_sub_len:
    ; Subtract r6 from remaining length r4/r5
    rep #$30
    a16
    x16
    lda r6
    beq .copy_sub_64k

    ; r6 != 0: subtract r6 from r4/r5
    lda r4
    sec
    sbc r6
    sta r4
    sep #$20
    a8
    lda r5
    sbc #$00
    sta r5
    bra .copy_loop

.copy_sub_64k:
    ; r6 == 0 (64KB transferred): decrement r5
    sep #$20
    a8
    dec r5
    bra .copy_loop

.copy_exit:
    plp
    rep #$30
    a16
    x16
    rts

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






