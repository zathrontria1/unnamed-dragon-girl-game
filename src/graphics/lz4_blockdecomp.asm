;vcprmin=10000
	section	"DONTMERGE_text.far._LZ4_DecompressBlock.0","acrx"
	a16
	x16
	global	_LZ4_DecompressBlock
    ; uint16_t LZ4_DecompressBlock(const uint8_t *src, uint8_t *dest, uint16_t block_size, uint16_t hdmaen);
    ; On enter (via __reg):
    ; r0/r1 = src (loword / bank)
    ; r2/r3 = dest (loword / bank)
    ; r10   = block_size
    ; r11   = hdmaen
    
    ; NOTE: cross bank read and write dests are not supported.
    ;       this matches behaviour with C code which ultimately
    ;       uses MVN or DMA, which has the underlying limitation.
_LZ4_DecompressBlock:
    a16
    x16

    lda r10
    bne .valid_block
    ; Block size is 0. Abort.
    lda #$0000
    rtl

.valid_block:
    lda r0
    sec
    sbc #$0004 ; Adjust source offset backward by 4 bytes for libSFX
    tax

    ldy r2

    sep #$20
    a8

    lda r3
    xba
    lda r1     ; B = dest bank, A = source bank

    jsl SFX_LZ4_decompress_block

    ; Return to C ABI in 16-bit mode (a16, x16)
    rep #$30
    a16
    x16

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
        
        ; Check if HDMA is enabled.
        lda     r11
        bne     .mvn_mode
        a16
        x16
        rep #$30

        jsr     DecodeBlock_DMA

        bra .decompress_end

        .mvn_mode:
        a16
        x16
        rep #$30

        jsr     DecodeBlock

        .decompress_end:
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

DecodeBlock_DMA:
        a16
        x16

        ldy     r2         ;Store destination offset for decompressed size calculation
        phy
        lda     [r0]     ;Read lower 16 bits of block size
        jsr     Skip4           ;Skip block size
        clc
        adc     r0       ;Store block end offset
        sta     r8

ReadToken_DMA:
        lda     [r0]     ;Read token byte
        pha                     ;Save for @Match
        inc     r0

        and     #$00f0          ;Check high nibble
        beq     .IsBlockDone_DMA    ;Zero: No literal

.Literal_DMA:
        lsr                     ;Compute literal length
        lsr
        lsr
        lsr
        cmp     #$000f          ;Short literal?
        bne     .CopyLiteral_DMA
        jsr     AddLength

.CopyLiteral_DMA:
        ; dma code
        ; Since most of the DMA setup is already done beforehand, just make sure the values make sense

        ; take advantage of the fact that DMA regs are R/W
        ; and directly use them without a function call.
        pha
        sta $4375 ; len

        lda r0
        sta $4372 ; src

        lda r2
        sta $2181 ; dest

        a8
        sep #$20

        lda #$80
        sta $420b

        a16
        rep #$21

        ldx $4372
        stx r0

        pla
        adc r2
        sta r2
        tay

        stx     r0       ;Copy offsets
        sty     r2

.IsBlockDone_DMA:
        lda     r8
        cmp     r0
        beq     BlockDone

.Match_DMA:
        pla                     ;Pull block token
        tax                     ;Stash

        lda     [r0]     ;Read match offset (word)
        pha                     ;and save on stack for @CopyMatch
        inc     r0
        inc     r0

        txa                     ;Swap back token
        and     #$000f          ;Check low nibble
        cmp     #$000f          ;Short match length?
        bne     .CopyMatch_DMA
        jsr     AddLength

.CopyMatch_DMA:
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

        bra     ReadToken_DMA

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
        jsr     AddLength

.CopyLiteral:
        ; original block move code
        :
        ldx     r0       ;Length in A, perform block move
        ldy     r2
        dec
        phb
        jsl     r4          ;Mode 21 = JSL
        plb
        ; end of original code
        stx     r0       ;Copy offsets
        sty     r2

.IsBlockDone:
        lda     r8
        cmp     r0
        beq     BlockDone

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
        jsr     AddLength

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


BlockDone:
        pla
        lda     r2         ;Calculate decompressed size
        sec
        sbc     1,s             ;Start offset on stack
        plx                     ;Unwind
        rts


AddLength:
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
        global  _DmaSystem_CopyToWram_ShortRun