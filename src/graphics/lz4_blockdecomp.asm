;vcprmin=10000
	section	"DONTMERGE_text.far._LZ4_DecompressBlock.0","acrx"
	a16
	x16
	global	_LZ4_DecompressBlock
    ; uint32_t LZ4_DecompressBlock(uint8_t **ptr_read, uint8_t **ptr_write, uint16_t block_size, uint16_t hdmaen);
    ; On enter:
    ; A/X = pointer to ptr_read
    ; 4,s = pointer to ptr_write
    ; 8,s = block_size
    ; 10,s = hdmaen
    
    ; NOTE: cross bank read and write dests are not supported.
    ;       this matches behaviour with C code which ultimately
    ;       uses MVN or DMA, which has the underlying limitation.
_LZ4_DecompressBlock:
    ; translate things from what vbcc/vasm expects to the new code.
    ; can carve out r0-r9, that's 18 bytes of zero page. As they're assumed
    ; to be volatile we can just use it
    ; use r10-15 + r28-31 for scratch here. Also volatile, so preservation isn't needed.
    rep #$30 
    
    sta r28
    stx r29 ; Immediately safe them as these registers will be used very soon

    ; fetch block size and HDMAEN enable
    ; r10 and r11 can be reused
    lda 8,s
    bne :+
        ; Block size is 0. Abort.
        lda #$0000
        ldx #$0000
        rtl
    :
    sta r10
    lda 10,s
    sta r11 ; TODO: HDMAEN is not read and always uses MVN even if safe.

    ; Fetch ptr_read
    lda [r28]
    sec
    sbc #$0004 ; Adjust it backwards by 4. Cross bank isn't a factor, so we just subtract 4

    ldy #$0002
    sta r12
    lda [r28],y
    sta r13 ; r12-13 contains ptr_read

    ; Fetch ptr_write
    lda 4,s
    sta r30
    lda 6,s
    sta r31 
    lda [r30]
    sta r14
    lda [r30],y
    sta r15 ; r14-15 contains ptr_write

    ; All variables have been copied

    ; now to put the variables in the format the subroutine expects.

    ldx r12
    ldy r14

    a8
    x16
    sep #$20
    rep #$10

    lda r15
    xba
    lda r13

    jsl SFX_LZ4_decompress_block

    ; After the subroutine returns, need to adjust read and write pointers

    a16
    x16
    rep #$20

    ; Update write pointer
    tax ; store A in X temporarily
    clc
    adc [r30]
    sta [r30]

    ; Update read pointer
    lda [r28]
    clc
    adc r10
    sta [r28]

    txa
    ldx #$0000 ; make the high bytes always 0

    rtl

; ported code directly below

;  https://github.com/Optiroc/libSFX/blob/master/include/Packages/LZ4/LZ4.s

;  SFX_LZ4_decompress_block
;  Decompress LZ4 block
;  [a8i16, ret:a16i16]
;
;  :in:  x       Source offset
;  :in:  y       Destination offset
;  :in:  b:a     Destination:Source banks
;  :out: a       Decompressed length

SFX_LZ4_decompress_block:
        a8
        x16

        jsr     Setup
        
        a16
        x16
        rep #$30

        jsr     DecodeBlock
        rtl


;-------------------------------------------------------------------------------
;Scratch pad usage
;.define LZ_source   ZPAD+$00    ;Source (indirect long)
;.define LZ_dest     ZPAD+$03    ;Destination (indirect long)
;.define LZ_mvl      ZPAD+$06    ;Literal block move (mvn + banks + return)
;.define LZ_mvm      ZPAD+$0a    ;Match block move (mvn + banks + return)
;.define LZ_blockend ZPAD+$0e    ;End address for current block

; LZ_source   = r0-r1
; LZ_dest     = r2-r3
; LZ_mvl      = r4-r5
; LZ_mvm      = r6-r7
; LZ_blockend = r8

Setup:
        a8
        x16

        stx     r0+$00   ;Set source for indirect and block move addressing
        sta     r0+$02
        sta     r4+$02

        xba                     ;Set destination for indirect and block move addressing
        sty     r2+$00
        sta     r2+$02
        sta     r4+$01
        sta     r6+$01
        sta     r6+$02

        lda     #$54            ;Write MVN and RTS/RTL instructions
        sta     r4+$00
        sta     r6+$00

        lda     #$6b            ;Mode 21 = RTL

        sta     r4+$03
        sta     r6+$03
        rts

DecodeBlock:
        a16
        x16

        ldy     r2         ;Store destination offset for decompressed size calculation
        phy
        lda     [r0]     ;Read lower 16 bits of block size
        jsr     Skip4           ;Skip block size
        clc
        adc     r0       ;Store block end offset
        sta     r8


ReadToken:
        lda     [r0]     ;Read token byte
        pha                     ;Save for @Match
        inc     r0

        and     #$00f0          ;Check high nibble
        beq     .IsBlockDone    ;Zero: No literal

.Literal:
        lsr                     ;Compute literal length
        lsr
        lsr
        lsr
        cmp     #$000f          ;Short literal?
        bne     .CopyLiteral
        jsr     .AddLength

.CopyLiteral:
        ldx     r0       ;Length in A, perform block move
        ldy     r2
        dec
        phb
        jsl     r4          ;Mode 21 = JSL
        plb
        stx     r0       ;Copy offsets
        sty     r2


.IsBlockDone:
        lda     r8
        cmp     r0
        beq     .BlockDone


.Match:
        pla                     ;Pull block token
        tax                     ;Stash

        lda     [r0]     ;Read match offset (word)
        pha                     ;and save on stack for @CopyMatch
        inc     r0
        inc     r0

        txa                     ;Swap back token
        and     #$000f          ;Check low nibble
        cmp     #$000f          ;Short match length?
        bne     .CopyMatch
        jsr     .AddLength

.CopyMatch:
        tay                     ;Length in A
        lda     r2         ;Copy from dest
        sec
        sbc     1,s             ;Offset on stack
        tax
        pla                     ;Unwind

        tya
        clc
        adc     #$03
        ldy     r2
        phb
        jsl     r6          ;Mode 21 = JSL
        plb
        sty     r2         ;Copy destination offset

        bra     ReadToken


.BlockDone:
        pla
        lda     r2         ;Calculate decompressed size
        sec
        sbc     1,s             ;Start offset on stack
        plx                     ;Unwind
        rts


.AddLength:
        pha                     ;Accumulated length at s+1
:       lda     [r0]     ;Read next length byte
        inc     r0
        tay
        and     #$00ff          ;Add to length
        clc
        adc     1,s
        sta     1,s

        tya                     ;Check end condition: length byte != #$ff
        a8
        sep #$20
        inc
        a16
        rep #$20
        beq     :-

        pla                     ;Done: pull back summed length
        rts


Skip4:  inc     r0       ;Skip 4 bytes
Skip3:  inc     r0       ;Skip 3 bytes
        inc     r0
        inc     r0
        rts

; stacksize=0+??
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
