.segment "SPCIMAGE"

_ins_play_note:
    ;(uint8_t id, uint8_t note, uint8_t vol_l, uint8_t vol_r)
    ; A contains instrument, X contains the current note
    ; r8 and r9 contain volume
    mov <r0, A ; sfx ID; do not immediately place in dsp_param_srcn yet

    mov <r1, X ; MIDI note number
    mov <r10, X ; also make a copy here

    mov <dsp_param_vol_l, <r8
    mov <dsp_param_vol_r, <r9 ; channel volumes; frees up these ZP variables

    ; check if volume is zero. if yes, return immediately
    mov A, <r8
    or A, <r9
    bne :+
        ret
    :

    ; subtract the MIDI note with reference note point 
    @note_adjust:
    mov A, <r1
    mov X, <r0
    setc 
    sbc A, !global_ins_tune+X
    mov <r1, A

    mov <dsp_param_vpitch, #$00
    mov <dsp_param_vpitch+1, #$10 ; initial pitch

    ; Now load <r1 and check if positive or negative
    mov A, <r1
    bpl :+
        ; negative (lower)
        eor A, #$ff
        inc A
        mov <r8, A ; Has the offset

        ; perform modulo 
        mov Y, #0
        mov A, <r8
        mov X, #12

        div ya,x ; modulo is stored in y
        mov <r9, Y 
        mov <r9+1,#0

        asl <r9
        mov X,<r9
        mov A,!lut_note_offset_neg+X
        mov <r10,A
        mov A,!lut_note_offset_neg+1+X
        mov <r10+1,A

        movw ya,<dsp_param_vpitch
        setc
        subw ya,<r10
        movw <dsp_param_vpitch,ya

        ; Adjust the pitch mod with the octave (always $1000 to start with)
        @shift_loop_neg:
        cmp <r8, #0
        bmi @shift_skip_neg
        cmp <r8, #12
        bcc @shift_skip_neg
            setc 
            sbc <r8, #12

            lsr <dsp_param_vpitch+1
            ror <dsp_param_vpitch
            bra @shift_loop_neg
        @shift_skip_neg:
        bra :++
    :
        ; positive (higher)
        mov <r8, A ; Has the offset

        ; perform modulo 
        mov Y, #0
        mov A, <r8
        mov X, #12

        div ya,x ; modulo is stored in y
        mov <r9, Y 
        mov <r9+1,#0

        asl <r9
        mov X,<r9
        mov A,!lut_note_offset_pos+X
        mov <r10,A
        mov A,!lut_note_offset_pos+1+X
        mov <r10+1,A

        movw ya,<dsp_param_vpitch
        clrc
        addw ya,<r10
        movw <dsp_param_vpitch,ya

        ; Adjust the pitch mod with the octave (always $1000 to start with)

        @shift_loop_pos:
        cmp <r8, #0
        bmi @shift_skip_pos
        cmp <r8, #12
        bcc @shift_skip_pos
            setc 
            sbc <r8, #12

            asl <dsp_param_vpitch
            rol <dsp_param_vpitch+1
            bra @shift_loop_pos
        @shift_skip_pos:
    :


    ; Fetch the slot effective value; needed for ADSR (auto fetched in _start_note)
    mov <r0+1, #0 ; clear the high byte
    mov <dsp_param_srcn, <r0 ; save this elsewhere as it'll be overwritten

    asl <r0
    rol <r0+1
    movw ya, <r0
    movw <r15, ya ; pointer will be used later by ADSR calcs; cannot be clobbered

    mov A, #<global_sfx_samplerates
    mov Y, #>global_sfx_samplerates
    clrc
    addw ya,<r0
    movw <r0,ya

    mov Y,#0

    mov A,[<r0]+Y
    mov <r9, A
    inc y
    mov A,[<r0]+Y
    mov <r9+1, A ; <r9 contains the sample's native rate.

    ; perform multiplication
    ; (adjusted vxpitch * native rate) >> 12
    movw ya, <dsp_param_vpitch
    movw <mul16_a, ya

    movw ya, <r9
    movw <mul16_b, ya

    call !_mul_16_by_16

    ; adjust the resulting number
    lsr <mul16_product_32+2
    ror <mul16_product_32+1
    lsr <mul16_product_32+2
    ror <mul16_product_32+1
    lsr <mul16_product_32+2
    ror <mul16_product_32+1
    lsr <mul16_product_32+2
    ror <mul16_product_32+1

    mov A, <mul16_product_32+3
    and A, #%00001111
    xcn A
    or A, <mul16_product_32+2
    mov <mul16_product_32+2, A

    movw ya, <mul16_product_32+1
    mov <mul16_product_32, #$ff
    mov <mul16_product_32+1, #$3f
    cmpw ya, <mul16_product_32
    bcc :+
        movw ya, <mul16_product_32
    :
    movw <dsp_param_vpitch,ya

    call !_start_note

    ; Update channel LRUs to the newest situation without ticking
    mov A, #0
    call !_update_channel_lru

    ret

_ins_play_oneshot:
    ;(uint8_t id, uint8_t vol_l, uint8_t vol_r)
    ; use for e.g. drums
    mov <r0, A ; sfx ID; do not immediately place in dsp_param_srcn yet

    mov <dsp_param_vol_l, <r8
    mov <dsp_param_vol_r, <r9 ; channel volumes; frees up these ZP variables

    ; check if volume is zero. if yes, return immediately
    mov A, <r8
    or A, <r9
    bne :+
        ret
    :

    ; Fetch the slot effective value; needed for ADSR (auto fetched in _start_note)
    mov <r0+1, #0 ; clear the high byte
    mov <dsp_param_srcn, <r0 ; save this elsewhere as it'll be overwritten

    asl <r0
    rol <r0+1
    movw ya, <r0
    movw <r15, ya ; pointer will be used later by ADSR calcs; cannot be clobbered

    mov A, #<global_sfx_samplerates
    mov Y, #>global_sfx_samplerates
    clrc
    addw ya,<r0
    movw <r0,ya

    mov Y,#0

    mov A,[<r0]+Y
    mov <dsp_param_vpitch, A
    inc y
    mov A,[<r0]+Y
    mov <dsp_param_vpitch+1, A ; <r9 contains the sample's native rate.

    call !_start_note

    ; Update channel LRUs to the newest situation without ticking
    mov A, #0
    call !_update_channel_lru

    ret

_sfx_play:
    ;(uint8_t id, uint8_t properties)
    mov <dsp_param_srcn, <REG_APUIO2 ; sfx ID
    mov A, <REG_APUIO3 ; pan (from -127 to +127)

    mov <REG_APUIO1,#SND_CMD_SFX_PLAY

    mov <r1,A
    bpl @pan_right

    @pan_left:
        ; negative pan value.
        mov <dsp_param_vol_l, #31 ; max reasonable left

        mov A, #127
        clrc
        adc A, <r1 ; becomes a subtraction
        lsr A
        lsr A
        mov <dsp_param_vol_r, A

        bra @pan_calc_done

    @pan_right:
        ; positive pan value
        mov <dsp_param_vol_r, #31 ; max reasonable right

        mov A, #127
        setc
        sbc A, <r1
        lsr A
        lsr A
        mov <dsp_param_vol_l, A

    @pan_calc_done:

    ; Fetch the slot effective value
    mov <r0, <dsp_param_srcn ; copy the SFX ID
    mov <r0+1, #0 ; clear the high byte

    asl <r0
    rol <r0+1
    movw ya, <r0
    movw <r15, ya ; pointer will be used later by ADSR calcs; cannot be clobbered

    mov A, #<global_sfx_samplerates
    mov Y, #>global_sfx_samplerates
    clrc
    addw ya,<r0
    movw <r0,ya

    mov Y,#0

    mov A,[<r0]+Y
    mov <dsp_param_vpitch, A
    inc y
    mov A,[<r0]+Y
    mov <dsp_param_vpitch+1, A

    call !_start_note

    ; Update channel LRUs to the newest situation without ticking
    mov A, #0
    call !_update_channel_lru

    ret

_sfx_play_extend:
    ;(uint8_t sfx_id, int8_t vol_l, int8_t vol_r, int8_t pitch)
    mov <dsp_param_srcn, <REG_APUIO2 ; sfx ID
    mov <dsp_param_vpitch, <REG_APUIO3 ; pitch (8-bit)

    mov <REG_APUIO1,<REG_APUIO1 ;echo the opcode.

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_SFX_PLAY_EXTEND_VOLDATA
    :
        cmp A,<REG_APUIO1
        bne :-

    ; then the left and right volume data
    mov <dsp_param_vol_r,<REG_APUIO3
    mov <dsp_param_vol_l,<REG_APUIO2

    mov <REG_APUIO1,<REG_APUIO1 ; again

    ; check if volume is zero. if yes, return immediately
    mov A, <dsp_param_vol_r
    or A, <dsp_param_vol_l
    bne :+
        ret
    :

    ; calculate the pitch (expand from 8 bits to 12 bits)
    ; shift both values to the left 4 times
    ; 0xff being equivalent to 0x1000
    mov <dsp_param_vpitch+1, #0 ; clear the high byte

    ; first increment it (0 is meaningless)
    inc <dsp_param_vpitch
    bne :+
        inc <dsp_param_vpitch+1
    :

    ; now shift it 3 times
    mov X,#3
    mov A,<dsp_param_vpitch
    :
        asl A
        rol <dsp_param_vpitch+1
        dec X
        bne :-
    mov <dsp_param_vpitch,A

    call !_start_note

    ; Update channel LRUs to the newest situation without ticking
    mov A, #0
    call !_update_channel_lru

    ret

; Call after finishing setting up non-shared input to a DSP channel
; to play the SFX or note
; tickdown rate and channel use are the same setup
; so they come here
_start_note:
    ; Update ADSR
    mov A, <seq_note_prefix
    beq @adsr_skip
        mov A, <seq_note_adsr_override
        or A, <seq_note_adsr_override+1
        beq @adsr_skip ; if ADSR is not set use defaults
        mov <dsp_param_adsr, <seq_note_adsr_override
        mov <dsp_param_adsr+1, <seq_note_adsr_override+1
        bra @adsr_end
    @adsr_skip:
        mov A, #<global_sfx_adsr
        mov Y, #>global_sfx_adsr
        clrc
        addw ya,<r15
        movw <r15,ya

        mov Y,#0

        mov A,[<r15]+Y
        mov <dsp_param_adsr, A
        inc y
        mov A,[<r15]+Y
        mov <dsp_param_adsr+1, A
    @adsr_end:

    ; Set the tickdown rate
    ; <dsp_param_srcn has the sfx id slot

    mov A, <seq_note_prefix ; check if the tick count is overridden
    beq @tick_skip
        mov A, <seq_note_tick_override
        bne @tick_end ; if tick is not set, use defaults
        ;beq @tick_skip ; if tick is not set, use defaults
            ;mov <r14, <seq_note_tick_override
            ;bra @tick_end
    @tick_skip:
        mov Y,<dsp_param_srcn
        mov A,!global_sfx_tickcounts+Y
        ;mov <r14,A 
    @tick_end:

    ;mov <r14, A ; store the sound's tickcount temporarily

    ; reset the extension command
    mov <seq_note_prefix, #0

    ; now we have to determine what channel to use.
    
    mov Y, <global_sfx_endsoonest

    mov !global_sfx_tick_counter+Y,A ; store the sound tickcount now

    mov A, !lut_channel_mask+Y

    ;mov <dsp_param_kon,<global_sfx_endsoonest
    ;mov A,#1
    ;cmp <dsp_param_kon,#0
    ;beq @skip_shift
    ;dec <dsp_param_kon
    ;@shift:
    ;    asl A
    ;    dec <dsp_param_kon
    ;    bpl @shift
    ;@skip_shift:

    mov <dsp_param_kon,A
    ; dsp_param_kon contains the channel to key on 

    ;mov Y,<global_sfx_endsoonest
    ;mov A,<r14
    ;mov !global_sfx_tick_counter+Y,A

    ;mov A,Y
    mov A,<global_sfx_endsoonest
    xcn A ; Contains the channel offset
    clrc
    adc A, #DSP_V0VOLL ; Contains the first channel
    mov <REG_DSPADDR, A
    mov <REG_DSPDATA, <dsp_param_vol_l

    ; Remaining DSPADDR reg can be incremented
    inc <REG_DSPADDR ; #DSP_V0VOLR
    mov <REG_DSPDATA, <dsp_param_vol_r

    inc <REG_DSPADDR ; #DSP_V0PL
    mov <REG_DSPDATA, <dsp_param_vpitch

    inc <REG_DSPADDR ; #DSP_V0PH
    mov <REG_DSPDATA, <dsp_param_vpitch+1

    inc <REG_DSPADDR ; #DSP_V0SRCN
    mov <REG_DSPDATA, <dsp_param_srcn

    inc <REG_DSPADDR ; #DSP_V0ADSRL
    mov <REG_DSPDATA, <dsp_param_adsr

    inc <REG_DSPADDR ; #DSP_V0ADSRH
    mov <REG_DSPDATA, <dsp_param_adsr+1

    inc <REG_DSPADDR ; #DSP_V0GAIN
    mov <REG_DSPDATA, #$7f ; always write this regardless

    ; This one is a global reg
    mov <REG_DSPADDR, #DSP_KON
    mov <REG_DSPDATA, <dsp_param_kon

    ret

