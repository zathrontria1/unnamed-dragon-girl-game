.include "spc-ca65.inc"
.include "sndeng_const.inc"
.include "sndeng_vars.inc"

.segment "SPCIMAGE"

;    Convention outside the stock IPL:
;
;    APU0: acknowledgement pipe, always check this before execution
;    APU1: opcode (used to echo to check if a command was sent correctly)
;    APU2-3: operands (2 bytes)
;
;    Some commands are multi-part; in this case, also wait for opcode echos.
;
;    APU0 should be used as echo/sync byte even with multi byte transfers.
;
;    For block transfers, uses same protocol as stock IPL upload.
;
;    When the SPC is ready to receive a new command,
;    APU0 will be set to $ff
;    APU1-3 will be set to $000000

; This should be at $0200
_start:
    ; Full clear zero page
    mov A, #$00
    mov Y, A ; $0000
    movw <r0,ya

    :
        mov [<r0]+Y,A
        inc Y
        cmp Y,#$f0
        bcc :-

    ; set up stack pointer
    mov X, #$ff
    mov sp,X

    ; Full clear stack
    inc <r0+1 ; increases to $01; makes $0100
    mov Y, A ; reset Y
    :
        mov [<r0]+Y,A
        inc Y
        bne :-

    ; Full clear BSS section
    mov <r0+1, #>BSS_START
    mov Y, A
    :
        mov [<r0]+Y,A
        inc Y
        bne :-
        ; if Y is 0, increment high byte of address
        inc <r0+1
        bne :- ; if address wraps all the way back to $0000, memory is fully cleared.

    ;Clear control ports
    mov <REG_CONTROL,#$30

    ; DSPInit start
    mov <REG_DSPADDR,#DSP_FLG
    mov <REG_DSPDATA,#$20 ; Unmute

    mov <REG_DSPADDR,#DSP_KON
    mov <REG_DSPDATA,A ; Reset Key on flags

    mov <REG_DSPADDR,#DSP_PMON
    mov <REG_DSPDATA,A ; Reset pitch modulation Enable Flags

    mov <REG_DSPADDR,#DSP_NON
    mov <REG_DSPDATA,A ; Reset Noise Enable Flags

    mov <REG_DSPADDR,#DSP_EON
    mov <REG_DSPDATA,A ; Reset Echo on Flags

    mov <REG_DSPADDR,#DSP_EVOLL
    mov <REG_DSPDATA,A ; Reset Echo Volume Left

    mov <REG_DSPADDR,#DSP_EVOLR
    mov <REG_DSPDATA,A ; Reset Echo Volume Right

    mov <REG_DSPADDR,#DSP_MVOL0L
    mov <REG_DSPDATA,#95 ; master volume left

    mov <REG_DSPADDR,#DSP_MVOL0R
    mov <REG_DSPDATA,#95 ; master volume right

    ; reset all CPU side read ports
    ; #$00 = #SND_SIG_CLEAR, so just use A
    mov <REG_APUIO0,A
    mov <REG_APUIO1,A
    mov <REG_APUIO2,A
    mov <REG_APUIO3,A

    ; set up the source directory
    mov <REG_DSPADDR,#DSP_DIR
    mov <REG_DSPDATA,#>global_sampletable

    ; set up the timer
    mov A, #255 ; also #$ff

    mov <REG_DSPADDR,#DSP_KOFF
    mov <REG_DSPDATA,A ; Set Key off flags; delayed to here so A is already 255

    mov <REG_T0DIV, #133 ; roughly 60Hz
    mov <REG_T1DIV, A ; placeholder slowest possible rate
    mov <REG_CONTROL, #$03 ; enable T0 and T1

    ; set up the tick timer
    mov <seq_tick_timer_target, A ; slowest possible additional tick wait. music is effectively stopped

    mov <global_last_cmd, A ; Make it so that the "last command" is the soft reset command which is impossible for a fresh boot

_main:
    ; Check if a signal is ready.
    mov A,<global_last_cmd
    cmp A,<REG_APUIO1
    bne :+
        ; no signal
        mov A, <REG_T0OUT
        beq @no_sfx_tick
            call !_process_sfx

        @no_sfx_tick:
        mov A, <REG_T1OUT
        beq @no_mus_tick
            mov A, <seq_tick_timer;
            inc A 
            mov <seq_tick_timer, A
            cmp A, <seq_tick_timer_target
            bcc @no_mus_tick
                call !_process_mus
                mov <seq_tick_timer, #0
        @no_mus_tick:

        bra _main
    :

    ; A signal exists. Check the contents of APUIO2 and jump accordingly
    mov A,<REG_APUIO1
    mov <global_received_cmd,A ;copy it so that it doesn't disappear on us later

    ; test the message
    cmp A,#SND_CMD_NOP
    bne :+
        mov <REG_APUIO1,#SND_CMD_NOP
        bra @end
    :

    ; a processing message, so the SPC is now considered busy
    mov <REG_APUIO0,#SND_SIG_BUSY

    ; as a rule
    ; opcode echo within the routine

    cmp A,#SND_CMD_DATA_UPLOAD
    bne :+
        call !_sfx_upload
        ; subroutine will twiddle the IO ports
        bra @end
    :
    cmp A,#SND_CMD_DATA_SAMPLE_SET_TUNE
    bne :+
        call !_set_tune
        bra @end
    :
    cmp A,#SND_CMD_SEQ_UPLOAD
    bne :+
        call !_mus_seq_upload
        ; subroutine will twiddle the IO ports
        bra @end
    :
    cmp A,#SND_CMD_SFX_PLAY
    bne :+
        call !_sfx_play
        bra @end
    :
    cmp A,#SND_CMD_SFX_PLAY_EXTEND
    bne :+
        call !_sfx_play_extend
        bra @end
    :
    cmp A,#SND_CMD_SFX_STOP
    bne :+
        call !_sfx_stop
        bra @end
    :
    cmp A,#SND_CMD_MUS_START
    bne :+
        call !_mus_start
        bra @end
    :
    cmp A,#SND_CMD_MUS_PAUSE
    bne :+
        call !_mus_pause
        bra @end
    :
    cmp A,#SND_CMD_MUS_STOP
    bne :+
        call !_mus_stop
        bra @end
    :
    cmp A,#SND_CMD_MUS_SET_TEMPO
    bne :+
        call !_mus_set_tempo
        bra @end
    :
    cmp A,#SND_CMD_DSP_SET
    bne :+
        call !_dsp_reg_write
        bra @end
    :
    cmp A,#SND_CMD_DIR_RESET
    bne :+
        call !_dir_reset
        bra @end
    :
    cmp A,#SND_CMD_SOFTRESET
    bne :+
        jmp !_reset_spc
    :
    @end:

    mov <global_last_cmd,<global_received_cmd

    mov <REG_APUIO0,#SND_SIG_CLEAR

    jmp !_main

_process_mus:
    mov A, <seq_playing
    bne :+
        ret
    :

    mov X, #0
    @track_loop:
    mov A, <seq_track_wait+X
    beq @track_active
        dec A
        mov <seq_track_wait+X,A
        jmp !@track_end
    @track_active:
    mov <seq_current_track, X ; save a copy here

    mov A, X
    asl A
    mov X, A ; pointer offset

    ; check if it's a valid track
    mov Y, <seq_ptr_start+1+X
    mov A, <seq_ptr_start+X

    mov <r0, #0
    mov <r0+1, #0
    cmpw ya, <r0
    bne :+
        ; invalid track
        mov X, <seq_current_track
        jmp !@track_end
    :

    mov A, <seq_ptr+X
    mov <seq_current_track_ptr, A
    mov A, <seq_ptr+1+X
    mov <seq_current_track_ptr+1, A
    
    mov Y, #0

    mov A, [<seq_current_track_ptr]+Y
    bmi :+ ; all below is a note
        mov X, A
        
        inc Y
        mov A, [<seq_current_track_ptr]+Y
        mov <r0, A

        inc Y
        mov A, [<seq_current_track_ptr]+Y
        mov <r8, A

        inc Y
        mov A, [<seq_current_track_ptr]+Y
        mov <r9, A

        mov A, <r0
        
        call !_ins_play_note

        jmp !@increment_track_ptr
    :
    cmp A, #SEQ_OPCODE_PLAY_ONESHOT
    bne :+
        inc Y
        mov A, [<seq_current_track_ptr]+Y
        mov <r0, A

        inc Y
        mov A, [<seq_current_track_ptr]+Y
        mov <r8, A

        inc Y
        mov A, [<seq_current_track_ptr]+Y
        mov <r9, A

        mov A, <r0
        
        call !_ins_play_oneshot

        jmp !@increment_track_ptr
    :
    cmp A, #SEQ_OPCODE_WAIT
    bne :+
        inc Y
        mov A, [<seq_current_track_ptr]+Y

        mov X, <seq_current_track
        mov <seq_track_wait+X, A

        jmp !@increment_track_ptr
    :
    cmp A, #SEQ_OPCODE_NOTEPREFIX
    bne :+
        mov <seq_note_prefix, #1
        inc Y
        mov A, [<seq_current_track_ptr]+Y
        mov <seq_note_adsr_override, A
        inc Y
        mov A, [<seq_current_track_ptr]+Y
        mov <seq_note_adsr_override+1, A
        inc Y
        mov A, [<seq_current_track_ptr]+Y
        mov <seq_note_tick_override, A

        call !_adjust_ptr

        mov X, <seq_current_track ; This must be placed outside the function

        jmp !@track_active
    :
    cmp A, #SEQ_OPCODE_SETRESTARTPOINT
    bne :+
        call !_adjust_ptr

        mov <seq_ptr_start+X, A 
        mov <seq_ptr_start+1+X, Y 

        mov X, <seq_current_track

        jmp !@track_active
    :
    cmp A, #SEQ_OPCODE_SETLOOPPOINT
    bne :+
        inc Y
        mov A, [<seq_current_track_ptr]+Y
        mov X, <seq_current_track
        mov <seq_loop_counter+X, A

        call !_adjust_ptr

        mov <seq_ptr_loop+X, A 
        mov <seq_ptr_loop+1+X, Y 

        mov X, <seq_current_track

        jmp !@track_active
    :
    cmp A, #SEQ_OPCODE_LOOP
    bne :+
        mov X, <seq_current_track
        mov A, <seq_loop_counter+X
        bne @prepare_loop_return
            ; Not looping
            call !_adjust_ptr

            mov X, <seq_current_track

            jmp !@track_active
        @prepare_loop_return:

        ; Special handling is done here. Do not call the subroutine!
        mov A, X
        asl A
        mov X, A

        mov A, <seq_ptr_loop+X
        mov <seq_ptr+X, A
        mov A, <seq_ptr_loop+1+X
        mov <seq_ptr+1+X, A

        mov X, <seq_current_track
        dec <seq_loop_counter+X

        jmp !@track_active ; go to normal processing
    :
    cmp A, #SEQ_OPCODE_RESTART
    bne :+
        ; Special handling is done here. Do not call the subroutine!

        mov A, <seq_ptr_start+X
        mov <seq_ptr+X, A
        mov A, <seq_ptr_start+1+X
        mov <seq_ptr+1+X, A

        mov X, <seq_current_track

        jmp !@track_active ; go to normal processing
    :

    @increment_track_ptr:

    call !_adjust_ptr

    mov X, <seq_current_track ; This must be placed outside the function
    
    @track_end:
    inc X
    cmp X, #8
    bcs @tick_processed
        jmp !@track_loop
    @tick_processed:


    ret

_adjust_ptr:
    ; Use this subroutine to adjust pointer
    ; A and Y will also contain the correct values 
    ; for writing out stuff when the subroutine returns
    mov A, <seq_current_track
    asl A
    mov X, A

    mov Y, <seq_ptr+1+X
    mov A, <seq_ptr+X

    mov <r0, #4
    mov <r0+1, #0

    clrc
    addw ya, <r0

    mov <seq_ptr+X, A 
    mov <seq_ptr+1+X, Y

    ; mov X, <seq_current_track ; This must be placed outside the function

    ret

_process_sfx:
    ; Perform all sfx tickdowns here
    mov A, #1 ; if A is set, tickdowns happen
    call !_update_channel_lru

    ret

    ; Stops all channels with the specified SFX
_sfx_stop:
    ;(uint8_t id)
    mov A,<REG_APUIO2
    mov <REG_APUIO1,#SND_CMD_SFX_STOP ; opcode echo

    mov X, #0
    mov <r1, X ; mask for KOFF

    mov <REG_DSPADDR, #DSP_V0SRCN ; Channel 0
    cbne <REG_DSPDATA, :+
        mov <global_sfx_tick_counter,X
        or <r1, #%00000001
    :
    clrc
    adc <REG_DSPADDR, #$10  ; Channel 1
    cbne <REG_DSPDATA, :+
        mov <global_sfx_tick_counter+1,X
        or <r1, #%00000010
    :
    clrc
    adc <REG_DSPADDR, #$10  ; Channel 2
    cbne <REG_DSPDATA, :+
        mov <global_sfx_tick_counter+2,X
        or <r1, #%00000100
    :
    clrc
    adc <REG_DSPADDR, #$10  ; Channel 3
    cbne <REG_DSPDATA, :+
        mov <global_sfx_tick_counter+3,X
        or <r1, #%00001000
    :
    clrc
    adc <REG_DSPADDR, #$10  ; Channel 4
    cbne <REG_DSPDATA, :+
        mov <global_sfx_tick_counter+4,X
        or <r1, #%00010000
    :
    clrc
    adc <REG_DSPADDR, #$10  ; Channel 5
    cbne <REG_DSPDATA, :+
        mov <global_sfx_tick_counter+5,X
        or <r1, #%00100000
    :
    clrc
    adc <REG_DSPADDR, #$10  ; Channel 6
    cbne <REG_DSPDATA, :+
        mov <global_sfx_tick_counter+6,X
        or <r1, #%01000000
    :
    clrc
    adc <REG_DSPADDR, #$10  ; Channel 7
    cbne <REG_DSPDATA, :+
        mov <global_sfx_tick_counter+7,X
        or <r1, #%10000000
    :

    ; This one is a global reg
    mov <REG_DSPADDR, #DSP_KOFF
    mov <REG_DSPDATA, <r1

    ; Update channel LRUs to the newest situation without ticking
    mov A, X ; X is #0
    call !_update_channel_lru

    ; Then restore it
    mov <REG_DSPADDR, #DSP_KOFF
    mov <REG_DSPDATA, #$00

    ret

; For this one, detect if it's called from _process or not
; if it ISN'T, do not perform tick downs
; can use a passed argument - in A
_update_channel_lru:
    ;(uint8_t tick_down)
    mov <r1,A

    mov A,#<global_sfx_tick_counter
    mov Y,#>global_sfx_tick_counter

    movw <r8,ya

    mov X,#8
    mov Y,#0

    mov <r0,#255

    @loop:
        mov A,[<r8]+Y
        cmp A,#255
        beq @check_next_channel
            ; tick is 255 (infinite)
            ; do nothing
        cmp A,#0
        beq @zero_tick
            ; Non-zero tick
            ; If the argument is set, decrement the value of A
            cmp <r1, #1
            bne @no_dec
                dec A
                mov [<r8]+Y,A
            @no_dec:
            ; Check if tick is the least recently used.
            cmp A, <r0
            bcs @check_next_channel
                mov <r0,A
                mov <global_sfx_endsoonest,Y
                bra @check_next_channel
        @zero_tick:
            ; Zero tick, aka finished playing.
            mov A, !lut_channel_mask+Y

            mov <REG_DSPADDR,#DSP_KOFF
            mov <REG_DSPDATA,A
            
            mov <global_sfx_endsoonest,Y

            mov <r0, #0 ; always true
        @check_next_channel:
        inc Y
        dec X
        bne @loop

    mov <REG_DSPADDR,#DSP_KOFF
    mov <REG_DSPDATA, #%000000000

    ret

_dir_reset:
    mov A, #<global_sampletable
    mov Y, #>global_sampletable

    movw <r8,ya
    inc Y
    movw <r9,ya
    inc Y
    movw <r10,ya
    inc Y
    movw <r11,ya

    mov Y,#0
    mov A,Y

    :
        mov [<r8]+Y,A
        mov [<r9]+Y,A
        mov [<r10]+Y,A
        mov [<r11]+Y,A
        inc Y
        cmp Y, #64
        bcc :-

    mov <global_nextfree, #0

    mov <REG_APUIO1,#SND_CMD_DIR_RESET

    ret


_dsp_reg_write:
    ;(uint8_t addr, uint8_t data)
    mov <REG_DSPADDR,<REG_APUIO2
    mov <REG_DSPDATA,<REG_APUIO3

    mov <REG_APUIO1,#SND_CMD_DSP_SET

    ret

_reset_spc:
    mov <REG_APUIO1,#SND_CMD_SOFTRESET
    
    mov <REG_CONTROL,#$80
    jmp !$ffc0

; multiplication routine for any sample rate tone to 32 bits result
; product is saved in <mul16_product_32
_mul_16_by_16:
    ;(uint16_t a, uint16_t b)
    mov A, <mul16_a
    mov Y, <mul16_b
    mul ya

    movw <mul16_product_32,ya
    mov <mul16_product_32+2, #0
    mov <mul16_product_32+3, #0

    mov A, <mul16_a
    mov Y, <mul16_b+1
    mul ya

    movw <mul16_scratch,ya

    mov A, <mul16_a+1
    mov Y, <mul16_b
    mul ya 

    clrc
    addw ya,<mul16_product_32+1
    addw ya,<mul16_scratch
    movw <mul16_product_32+1,ya

    mov A, <mul16_a+1
    mov Y, <mul16_b+1
    mul ya 
    movw <mul16_scratch,ya

    movw ya, <mul16_product_32+2
    bcc :+
        mov <mul16_fix, #$ff
        mov <mul16_fix+1, #$00
        addw ya, <mul16_fix
    :
    addw ya, <mul16_scratch
    movw <mul16_product_32+2,ya

    ret

_set_tune:
    ; (uint8_t ins_id, uint8_t tune);
    mov <r0, <REG_APUIO2
    mov <r1, <REG_APUIO3

    mov X, <r0
    mov A, <r1
    mov !global_ins_tune+X,A

    mov <REG_APUIO1,<REG_APUIO1

    ret

lut_note_offset_pos:
    .word 0
    .word 4340-4096
    .word 4598-4096
    .word 4871-4096
    .word 5161-4096
    .word 5468-4096
    .word 5793-4096
    .word 6137-4096
    .word 6502-4096
    .word 6889-4096
    .word 7298-4096
    .word 7732-4096

lut_note_offset_neg:
    .word 2170-2048
    .word 2299-2048
    .word 2435-2048
    .word 2580-2048
    .word 2734-2048
    .word 2896-2048
    .word 3069-2048
    .word 3251-2048
    .word 3444-2048
    .word 3649-2048
    .word 3866-2048
    .word 2048
    
lut_channel_mask:
    .byte %00000001
    .byte %00000010
    .byte %00000100
    .byte %00001000
    .byte %00010000
    .byte %00100000
    .byte %01000000
    .byte %10000000

.include "sndeng_play.asm"
.include "sndeng_mus.asm"
.include "sndeng_datareceive.asm"
