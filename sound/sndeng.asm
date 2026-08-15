.include "spc-ca65.inc"
.include "sndeng_const.inc"
.include "sndeng_vars.inc"

.export _start
.export _stream_upload
.export _data_upload_loop_stream
.export _data_upload_loop_2byte
.export _stream_play
.export _stream_stop

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

    ; Full clear all BSS tables, stream buffers, and sample pool from BSS_START to $FFFF
    ; Phase 1: Clear partial first page
    mov <r0, #0
    mov <r0+1, #>BSS_START
    mov Y, #<BSS_START
    mov A, #0
    :
        mov [<r0]+Y, A
        inc Y
        bne :-

    ; Phase 2: Clear all remaining full pages to $FFFF
    inc <r0+1
    :
        mov [<r0]+Y, A
        inc Y
        bne :-
        inc <r0+1
        bne :-

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
    mov <REG_DSPDATA,#96 ; master volume left

    mov <REG_DSPADDR,#DSP_MVOL0R
    mov <REG_DSPDATA,#96 ; master volume right

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

    ; write hardcoded values for the 63th entry of the sample table
    mov A, #<stream_data
    mov !global_sampletable+252, A
    mov !global_sampletable+254, A
    mov A, #>stream_data
    mov !global_sampletable+253, A
    mov !global_sampletable+255, A ; Set both to the same pointer to simulate a loop

    ; Place a guard header
    mov A, !stream_data+144
    or A, #$03
    mov !stream_data+144, A

    mov <REG_DSPADDR,#DSP_KOFF
    mov <REG_DSPDATA, #0 ; Clear all Key offs

    mov <REG_T0DIV, #133 ; roughly 60Hz
    mov A, #$ff
    mov <REG_T1DIV, A ; placeholder slowest possible rate
    mov <REG_CONTROL, #$03 ; enable T0 and T1

    ; set up the tick timer
    mov <seq_tick_timer_target, A ; slowest possible additional tick wait. music is effectively stopped
    mov <seq_speed, #6
    mov <seq_tick_in_row, #0

    mov A, #0
    mov <global_sfx_end, A
    mov <global_sfx_end+1, A

    mov A, #96
    mov !seq_music_volume, A
    mov !seq_sfx_volume, A
    mov !seq_voice_volume, A

    mov X, #0
    mov A, #$ff
    :
        mov !seq_track_channel+X, A
        mov !voice_owner+X, A
        inc X
        cmp X, #8
        bcc :-

    mov <global_last_cmd, A ; Make it so that the "last command" is the soft reset command which is impossible for a fresh boot
_main:
    call !_poll_command
    bne @have_command

    call !_poll_sfx_timer
    beq @no_sfx
        call !_process_sfx
    @no_sfx:

    call !_poll_music_timer
    beq @no_music
        call !_process_mus
        cmp A, #1
        beq @have_command
    @no_music:

    call !_poll_stream_watchdog
    beq _main
        call !_stream_stop
    bra _main

    @have_command:
    call !_service_command

    bra _main

; polling/check routines
_poll_command:
    ; Check if a signal is ready.
    mov A, <global_last_cmd
    cmp A, <REG_APUIO1
    beq @none
        mov A, #1
        ret
    @none:
    mov A, #0
    ret

_poll_sfx_timer:
    mov A, <REG_T0OUT
    ret

_poll_music_timer:
    mov A, <REG_T1OUT
    beq @none

    mov A, <seq_tick_timer
    inc A
    mov <seq_tick_timer, A
    cmp A, <seq_tick_timer_target
    bcc @none

    mov <seq_tick_timer, #0
    mov A, #1
    ret

    @none:
    mov A, #0
    ret

_poll_stream_watchdog:
    mov A, <stream_watchdog
    or A, <stream_watchdog+1
    beq @none

    mov <r15, #1
    mov <r15+1, #0
    movw ya, <stream_watchdog
    setc
    subw ya, <r15
    movw <stream_watchdog, ya

    mov A, <stream_watchdog
    or A, <stream_watchdog+1
    bne @none

    mov A, #1
    ret

    @none:
    mov A, #0
    ret

SND_CMD_COUNT = $16

_service_command:
    mov A, <REG_APUIO1
    beq @end_skipinc ; SND_CMD_NOP ($00) -> skip

    cmp A, #SND_CMD_COUNT + 1
    bcs @end_skipinc ; Out-of-range safety check

    dec A
    asl A
    mov X, A
    call !@indirect_dispatch
    inc <global_current_command_counter

@end_skipinc:
    mov <REG_CONTROL, #$33 ; Reset the read ports
    mov <global_last_cmd, #SND_CMD_NOP

    mov <REG_APUIO0, <global_current_command_counter
    mov <REG_APUIO1, #0
    mov <REG_APUIO2, #0
    mov <REG_APUIO3, #0
    ret

@indirect_dispatch:
    jmp_ [!@cmd_table+X]

@cmd_table:
    .word _sfx_upload           ; $01: SND_CMD_DATA_SAMPLE_UPLOAD
    .word _set_tune             ; $02: SND_CMD_DATA_SAMPLE_SET_TUNE
    .word _mus_seq_upload       ; $03: SND_CMD_SEQ_UPLOAD
    .word _sfx_play             ; $04: SND_CMD_SFX_PLAY
    .word _sfx_play_extend      ; $05: SND_CMD_SFX_PLAY_EXTEND
    .word _sfx_stop             ; $06: SND_CMD_SFX_STOP
    .word _mus_start            ; $07: SND_CMD_MUS_START
    .word _mus_pause            ; $08: SND_CMD_MUS_PAUSE
    .word _mus_stop             ; $09: SND_CMD_MUS_STOP
    .word _mus_set_tempo        ; $0A: SND_CMD_MUS_SET_TEMPO
    .word _mus_set_speed        ; $0B: SND_CMD_MUS_SET_SPEED
    .word _mus_set_outputmode   ; $0C: SND_CMD_MUS_SET_OUTPUTMODE
    .word _set_music_volume     ; $0D: SND_CMD_SET_MUSIC_VOL
    .word _set_sfx_volume       ; $0E: SND_CMD_SET_SFX_VOL
    .word _set_voice_volume     ; $0F: SND_CMD_SET_VOICE_VOL
    .word _stream_stop          ; $10: SND_CMD_STREAM_STOP
    .word _stream_upload        ; $11: SND_CMD_STREAM_UPLOAD
    .word _dsp_reg_write        ; $12: SND_CMD_DSP_SET
    .word _dir_reset            ; $13: SND_CMD_DIR_RESET
    .word _reset_spc            ; $14: SND_CMD_SOFTRESET
    .word _lock_sfx_boundary    ; $15: SND_CMD_LOCK_SFX
    .word _mus_ins_reset        ; $16: SND_CMD_MUS_INS_RESET

_process_mus:
    mov A, <seq_playing
    bne :+
        mov A, #0
        ret
    :

    ; Check if this tick is a Row Start (seq_tick_in_row == 0)
    mov A, <seq_tick_in_row
    bne @sub_row_tick

    mov X, <seq_process_track
    @track_loop:
    mov A, <global_last_cmd
    cmp A, <REG_APUIO1
    beq :+
        jmp !@yield_to_command
    :

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
    bpl @note ; all positive (0x00..0x7F) is a 1-byte note
    cmp A, #$90
    bcc @is_short_wait ; 0x80..0x8F is a 1-byte short wait (0..15 ticks)
    cmp A, #$ff
    beq @opcode_restart ; 0xFF is restart/loop track
    jmp !@opcode_runfromtable ; 0x90..0x9a are extended opcodes

    @track_end:
    inc X
    cmp X, #8
    bcs @tick_processed
        mov <seq_process_track, X
        jmp !@track_loop
    @tick_processed:
    mov A, #0
    mov <seq_process_track, A

    @sub_row_tick:
    inc <seq_tick_in_row
    mov A, <seq_tick_in_row
    cmp A, <seq_speed
    bcc :+
        mov <seq_tick_in_row, #0
    :
    call !_update_sub_row_pitch
    mov A, #0
    ret

@yield_to_command:
    mov <seq_process_track, X
    call !_service_command

    mov A, <seq_playing
    beq @yield_return

    mov X, <seq_process_track
    jmp !@track_loop

@yield_return:
    mov A, #0
    ret

@note:
    mov X, A ; X = note

    mov A, <seq_current_track
    mov Y, A
    mov A, !seq_track_vol_l+Y
    mov <r8, A
    mov A, !seq_track_vol_r+Y
    mov <r9, A
    mov A, !seq_track_ins+Y ; Instrument ID

    call !_ins_play_note

    ; Store initial note pitch into seq_track_pitch_curr
    mov A, <seq_current_track
    asl A
    mov X, A
    mov A, <dsp_param_vpitch
    mov !seq_track_pitch_curr+X, A
    mov A, <dsp_param_vpitch+1
    mov !seq_track_pitch_curr+1+X, A

    ; Reset pitch portamento mode for newly keyed notes
    mov X, <seq_current_track
    mov A, #0
    mov !seq_track_pitch_mode+X, A

    call !_advance_ptr_1
    jmp !@track_end

@is_short_wait:
    and A, #$0f
    mov X, <seq_current_track
    mov <seq_track_wait+X, A

    call !_advance_ptr_1
    jmp !@track_end

@opcode_runfromtable:
    setc
    sbc A, #$90
    asl A
    mov X, A
    jmp_ [!@seq_opcode_table+X]

@opcode_restart:
    mov A, <seq_current_track
    asl A
    mov X, A

    mov A, <seq_ptr_start+X
    mov <seq_ptr+X, A
    mov A, <seq_ptr_start+1+X
    mov <seq_ptr+1+X, A

    mov X, <seq_current_track
    jmp !@track_active ; go to normal processing

@opcode_wait_ext:
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov X, <seq_current_track
    mov <seq_track_wait+X, A

    call !_advance_ptr_2
    jmp !@track_end

@opcode_set_ins:
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov X, <seq_current_track
    mov !seq_track_ins+X, A

    call !_advance_ptr_2
    jmp !@track_active

@opcode_set_vol:
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov X, <seq_current_track
    mov !seq_track_vol_l+X, A
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov !seq_track_vol_r+X, A

    ; Check if this track owns an active voice
    mov A, !seq_track_channel+X
    cmp A, #8
    bcs @set_vol_done

    mov Y, A
    mov A, !voice_owner+Y
    cmp A, <seq_current_track
    bne @set_vol_done

    ; Active voice found in Y (channel 0..7)
    ; Save active voice channel in r0 (since _scale_volumes_music clobbers Y)
    mov <r0, Y

    ; Prepare volumes in dsp_param_vol_l/r
    mov A, !seq_track_vol_l+X
    mov <dsp_param_vol_l, A
    mov A, !seq_track_vol_r+X
    mov <dsp_param_vol_r, A

    ; Apply mono downmix if enabled
    mov A, !seq_force_mono
    beq :+
        mov A, <dsp_param_vol_l
        clrc
        adc A, <dsp_param_vol_r
        lsr A
        mov <dsp_param_vol_l, A
        mov <dsp_param_vol_r, A
    :

    call !_scale_volumes_music

    ; Calculate DSP address for voice (voice in r0, r0 * 16)
    mov A, <r0
    xcn A
    clrc
    adc A, #DSP_V0VOLL
    mov <REG_DSPADDR, A
    mov <REG_DSPDATA, <dsp_param_vol_l

    inc <REG_DSPADDR ; #DSP_V0VOLR
    mov <REG_DSPDATA, <dsp_param_vol_r

@set_vol_done:
    call !_advance_ptr_3
    jmp !@track_active

@opcode_set_adsr:
    mov A, <seq_current_track
    asl A
    mov X, A

    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov !seq_track_adsr_override+X, A
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov !seq_track_adsr_override+1+X, A

    call !_advance_ptr_3
    jmp !@track_active

@opcode_set_duration:
    mov A, <seq_current_track
    asl A
    mov X, A

    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov !seq_track_tick_override+X, A
    mov A, #0
    mov !seq_track_tick_override+1+X, A

    call !_advance_ptr_2
    jmp !@track_active

@opcode_play_drum:
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov <r0, A ; Drum ID

    mov A, <seq_current_track
    mov Y, A
    mov A, !seq_track_vol_l+Y
    mov <r8, A
    mov A, !seq_track_vol_r+Y
    mov <r9, A

    mov A, <r0
    call !_ins_play_oneshot

    call !_advance_ptr_2
    jmp !@track_end

@opcode_set_loop:
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov X, <seq_current_track
    mov <seq_loop_counter+X, A

    call !_advance_ptr_2

    mov A, <seq_current_track
    asl A
    mov X, A
    mov A, <seq_ptr+X
    mov <seq_ptr_loop+X, A
    mov A, <seq_ptr+1+X
    mov <seq_ptr_loop+1+X, A

    mov X, <seq_current_track
    jmp !@track_active

@opcode_loop:
    mov X, <seq_current_track
    mov A, <seq_loop_counter+X
    beq @loop_done
        dec <seq_loop_counter+X

        mov A, X
        asl A
        mov X, A
        mov A, <seq_ptr_loop+X
        mov <seq_ptr+X, A
        mov A, <seq_ptr_loop+1+X
        mov <seq_ptr+1+X, A

        mov X, <seq_current_track
        jmp !@track_active
    @loop_done:
        call !_advance_ptr_1
        jmp !@track_active

@opcode_set_restart:
    call !_advance_ptr_1

    mov A, <seq_current_track
    asl A
    mov X, A
    mov A, <seq_ptr+X
    mov <seq_ptr_start+X, A
    mov A, <seq_ptr+1+X
    mov <seq_ptr_start+1+X, A

    mov X, <seq_current_track
    jmp !@track_active

@opcode_set_speed:
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    bne :+
        mov A, #6
    :
    mov <seq_speed, A

    call !_advance_ptr_2
    jmp !@track_active

@opcode_set_tempo:
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov <REG_T1DIV, A

    call !_advance_ptr_2
    jmp !@track_active

@opcode_note_cut:
    call !_advance_ptr_1

    mov X, <seq_current_track
    mov A, !seq_track_channel+X
    cmp A, #8
    bcs @cut_done

    mov Y, A
    mov A, !voice_owner+Y
    cmp A, <seq_current_track
    bne @cut_done

    ; Issue KOFF to this voice channel
    mov A, !lut_channel_mask+Y
    mov <REG_DSPADDR, #DSP_KOFF
    mov <REG_DSPDATA, A

    ; Clear tick counter for this voice
    mov A, Y
    asl A
    mov X, A
    mov A, #0
    mov <global_sfx_tick_counter+X, A
    mov <global_sfx_tick_counter+1+X, A

    ; Clear tracking
    mov X, <seq_current_track
    mov A, #$ff
    mov !seq_track_channel+X, A
    mov !voice_owner+Y, A

    ; Clear KOFF and update LRU
    mov <REG_DSPADDR, #DSP_KOFF
    mov <REG_DSPDATA, #0

    mov A, #0
    call !_update_channel_lru

@cut_done:
    mov X, <seq_current_track
    jmp !@track_active

@opcode_call_sub:
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov <r0, A
    inc Y
    mov A, [<seq_current_track_ptr]+Y
    mov <r0+1, A

    ; Return address is seq_ptr + 3
    mov A, <seq_current_track
    asl A
    mov X, A

    mov A, <seq_ptr+X
    clrc
    adc A, #3
    mov <seq_track_ret+X, A
    mov A, <seq_ptr+1+X
    adc A, #0
    mov <seq_track_ret+1+X, A

    ; Target address is seq_ptr_start + r0 (relative offset)
    mov A, <seq_ptr_start+X
    clrc
    adc A, <r0
    mov <seq_ptr+X, A
    mov A, <seq_ptr_start+1+X
    adc A, <r0+1
    mov <seq_ptr+1+X, A

    mov X, <seq_current_track
    jmp !@track_active

@opcode_ret:
    ; Restore seq_ptr from seq_track_ret
    mov A, <seq_current_track
    asl A
    mov X, A

    mov A, <seq_track_ret+X
    mov <seq_ptr+X, A
    mov A, <seq_track_ret+1+X
    mov <seq_ptr+1+X, A

    mov X, <seq_current_track
    jmp !@track_active

@seq_opcode_table:
    .word @opcode_wait_ext     ; $90
    .word @opcode_set_ins      ; $91
    .word @opcode_set_vol      ; $92
    .word @opcode_set_adsr     ; $93
    .word @opcode_set_duration ; $94
    .word @opcode_play_drum    ; $95
    .word @opcode_set_loop     ; $96
    .word @opcode_loop         ; $97
    .word @opcode_set_restart  ; $98
    .word @opcode_set_speed    ; $99
    .word @opcode_set_tempo    ; $9a
    .word @opcode_note_cut     ; $9b
    .word @opcode_call_sub     ; $9c
    .word @opcode_ret          ; $9d
    .word @opcode_set_porta    ; $9e
    .word @opcode_set_pitch_slide ; $9f

@opcode_set_pitch_slide:
    ; 3-byte opcode: $9F, delta_low, delta_high (signed 16-bit DSP pitch delta per sub-tick)
    ; Positive delta = slide up (Fxx), Negative (two's complement) = slide down (Exx)
    ; delta = 0x0000 = stop slide (set mode to 0)
    inc Y
    mov A, [<seq_current_track_ptr]+Y  ; delta_low
    mov <r12, A
    inc Y
    mov A, [<seq_current_track_ptr]+Y  ; delta_high
    mov <r13, A

    ; Store signed delta into seq_track_pitch_slide
    mov A, <seq_current_track
    asl A
    mov X, A
    mov A, <r12
    mov !seq_track_pitch_slide+X, A
    mov A, <r13
    mov !seq_track_pitch_slide+1+X, A

    ; Determine mode: zero delta -> stop (mode 0); nonzero -> free slide (mode 1)
    mov A, <r12
    or A, <r13
    mov X, <seq_current_track
    beq :+
        mov A, #1
        mov !seq_track_pitch_mode+X, A
        bra :++
    :
        mov A, #0
        mov !seq_track_pitch_mode+X, A
    :

    call !_advance_ptr_3
    jmp !@track_active

@opcode_set_porta:
    ; 3-byte opcode: $9E, target_note, speed
    inc Y
    mov A, [<seq_current_track_ptr]+Y ; Target note
    mov <r12, A ; Save target note in r12
    inc Y
    mov A, [<seq_current_track_ptr]+Y ; Speed
    mov <r13, A ; Save speed in r13

    ; Calculate target note DSP pitch using _lookup_pitch_note
    mov X, <r12 ; X = target note
    mov A, <seq_current_track
    mov Y, A
    mov A, !seq_track_ins+Y ; Instrument ID
    call !_lookup_pitch_note

    ; Store calculated target pitch into seq_track_pitch_target
    mov A, <seq_current_track
    asl A
    mov X, A
    mov A, <dsp_param_vpitch
    mov !seq_track_pitch_target+X, A
    mov A, <dsp_param_vpitch+1
    mov !seq_track_pitch_target+1+X, A

    ; Convert speed in r13 to 16-bit pitch delta (speed * 32)
    mov A, <r13
    mov <r13+1, #0
    asl A
    rol <r13+1
    asl A
    rol <r13+1
    asl A
    rol <r13+1
    asl A
    rol <r13+1
    asl A
    rol <r13+1
    mov <r13, A

    mov A, <seq_current_track
    asl A
    mov X, A
    mov A, <r13
    mov !seq_track_pitch_slide+X, A
    mov A, <r13+1
    mov !seq_track_pitch_slide+1+X, A

    ; Set pitch mode to 2 (Portamento active)
    mov X, <seq_current_track
    mov A, #2
    mov !seq_track_pitch_mode+X, A

    call !_advance_ptr_3
    jmp !@track_active

_update_sub_row_pitch:
    mov <r14, #0  ; track index 0..7

@update_pitch_loop:
    mov X, <r14
    mov A, !seq_track_pitch_mode+X
    bne :+
        jmp !@next_pitch_track  ; mode == 0, nothing to do
    :

    ; mode != 0: check voice channel ownership
    mov A, !seq_track_channel+X
    cmp A, #8
    bcc :+
        jmp !@next_pitch_track
    :
    mov Y, A
    mov A, !voice_owner+Y
    cmp A, <r14
    beq :+
        jmp !@next_pitch_track
    :
    mov <r15, Y  ; save active voice channel

    ; Load curr pitch and slide delta (shared by both modes)
    mov A, <r14
    asl A
    mov X, A
    mov A, !seq_track_pitch_curr+X
    mov <r2, A
    mov A, !seq_track_pitch_curr+1+X
    mov <r2+1, A
    mov A, !seq_track_pitch_slide+X
    mov <r4, A
    mov A, !seq_track_pitch_slide+1+X
    mov <r4+1, A

    ; Dispatch on mode value
    mov X, <r14
    mov A, !seq_track_pitch_mode+X
    cmp A, #2
    bne :+
        jmp !@handle_portamento
    :

    ; Mode 1: free pitch slide (Exx/Fxx) -- add signed delta and clamp
    movw ya, <r2
    clrc
    addw ya, <r4       ; signed add (two's complement for downward slide)
    movw <r2, ya       ; store tentative result
    ; Clamp: high byte in r2+1
    mov A, <r2+1
    bmi @clamp_free_low    ; bit 7 set = wrapped below zero
    cmp A, #$40            ; >= 0x4000 = above DSP pitch ceiling
    bcc @free_slide_done
    ; Result > 0x3FFF: clamp to 0x3FFF
    mov <r2, #$ff
    mov <r2+1, #$3f
    bra @free_slide_done
@clamp_free_low:
    ; Result < 0: clamp to 0x0001 (avoid silencing voice)
    mov <r2, #$01
    mov <r2+1, #$00
@free_slide_done:
    bra @write_dsp_pitch

@handle_portamento:
    ; Mode 2: portamento toward target (Gxx)
    ; Load target pitch
    mov A, <r14
    asl A
    mov X, A
    mov A, !seq_track_pitch_target+X
    mov <r3, A
    mov A, !seq_track_pitch_target+1+X
    mov <r3+1, A

    ; Compare curr (r2) with target (r3)
    movw ya, <r2
    cmpw ya, <r3
    beq @porta_reached
    bcc @slide_pitch_up

    ; Slide Pitch Down (curr > target)
    setc
    subw ya, <r4
    ; Check if we went below target
    cmpw ya, <r3
    bcs :+
        ; Reached target
        mov X, <r14
        mov A, #0
        mov !seq_track_pitch_mode+X, A
        movw ya, <r3
    :
    movw <r2, ya
    bra @write_dsp_pitch

@slide_pitch_up:
    ; Slide Pitch Up (curr < target)
    clrc
    addw ya, <r4
    ; Check if we went above target
    cmpw ya, <r3
    bcc :+
        ; Reached target
        mov X, <r14
        mov A, #0
        mov !seq_track_pitch_mode+X, A
        movw ya, <r3
    :
    movw <r2, ya
    bra @write_dsp_pitch

@porta_reached:
    mov X, <r14
    mov A, #0
    mov !seq_track_pitch_mode+X, A
    bra @next_pitch_track

@write_dsp_pitch:
    ; Save updated curr pitch back to seq_track_pitch_curr
    mov A, <r14
    asl A
    mov X, A
    mov A, <r2
    mov !seq_track_pitch_curr+X, A
    mov A, <r2+1
    mov !seq_track_pitch_curr+1+X, A

    ; Write updated pitch to DSP voice (voice channel in r15)
    mov A, <r15
    xcn A
    clrc
    adc A, #DSP_V0PL
    mov <REG_DSPADDR, A
    mov <REG_DSPDATA, <r2

    inc <REG_DSPADDR ; DSP_V0PH
    mov <REG_DSPDATA, <r2+1

@next_pitch_track:
    inc <r14
    mov A, <r14
    cmp A, #8
    bcs :+
        jmp !@update_pitch_loop
    :
    ret

_advance_ptr_3:
    mov A, #3
    bra _advance_ptr_A

_advance_ptr_2:
    mov A, #2
    bra _advance_ptr_A

_advance_ptr_1:
    mov A, #1

_advance_ptr_A:
    mov <r0, A
    mov A, <seq_current_track
    asl A
    mov X, A
    mov A, <seq_ptr+X
    clrc
    adc A, <r0
    mov <seq_ptr+X, A
    bcc :+
        inc <seq_ptr+1+X
    :
    mov X, <seq_current_track
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
    bra _stop_voice_channels

_stream_stop:
    mov <stream_active, #0
    mov <stream_current_block, #0
    mov <stream_watchdog, #0
    mov <stream_watchdog+1, #0
    mov A, #63 ; voice for stream

_stop_voice_channels:
    mov <r0, A ; target SFX/voice ID to stop
    mov <r1, #0 ; mask for KOFF

    mov <REG_DSPADDR, #DSP_V0SRCN
    mov X, #0 ; tick counter byte offset (0, 2, 4, ... 14)
    mov Y, #0 ; channel index (0..7)
@chan_loop:
    mov A, <r0
    cbne <REG_DSPDATA, @next_chan
        mov A, #0
        mov <global_sfx_tick_counter+X, A
        mov <global_sfx_tick_counter+1+X, A
        mov A, !lut_channel_mask+Y
        or A, <r1
        mov <r1, A
@next_chan:
    clrc
    adc <REG_DSPADDR, #$10
    inc X
    inc X
    inc Y
    cmp Y, #8
    bcc @chan_loop

    ; This one is a global reg
    mov <REG_DSPADDR, #DSP_KOFF
    mov <REG_DSPDATA, <r1

    ; Update channel LRUs to the newest situation without ticking
    mov A, #0
    call !_update_channel_lru

    ; Then restore it
    mov <REG_DSPADDR, #DSP_KOFF
    mov <REG_DSPDATA, #$00

    ret

_update_channel_lru:
    ;(uint8_t tick_down)
    mov <r1, A          ; 1 = tickdown pass; 0 = query only
    mov <r3, #0         ; accumulated KOFF bitmask

    mov X, #0           ; Byte offset (0, 2, 4, ... 14)
    mov Y, #0           ; Channel index (0..7)
    mov <r0, #$ff       ; Min tick low byte
    mov <r0+1, #$ff     ; Min tick high byte

@loop:
    ; Check if tick is infinite ($FFFF)
    mov A, <global_sfx_tick_counter+1+X
    cmp A, #$ff
    bne @not_infinite
        mov A, <global_sfx_tick_counter+X
        cmp A, #$ff
        beq @next_chan  ; $FFFF -> infinite, do not steal
@not_infinite:

    ; Check if tick is zero (idle channel)
    mov A, <global_sfx_tick_counter+X
    or A, <global_sfx_tick_counter+1+X
    bne @not_zero
        ; Idle channel (0 ticks): top priority candidate
        mov <global_sfx_endsoonest, Y
        mov <r0, #0
        mov <r0+1, #0
        bra @next_chan

@not_zero:
    ; Non-zero tick: check if tickdown requested (r1 == 1)
    cmp <r1, #1
    bne @no_dec
        ; Tickdown pass: decrement 16-bit tick count
        mov A, <global_sfx_tick_counter+X
        bne :+
            dec <global_sfx_tick_counter+1+X
        :
        dec <global_sfx_tick_counter+X

        ; Check if it just reached 0
        mov A, <global_sfx_tick_counter+X
        or A, <global_sfx_tick_counter+1+X
        bne @no_dec
            ; Channel just expired: add to KOFF mask and mark as free
            mov A, !voice_owner+Y
            cmp A, #8
            bcs :+
                push X
                mov X, A
                mov A, #$ff
                mov !seq_track_channel+X, A
                pop X
            :
            mov A, #$ff
            mov !voice_owner+Y, A

            mov A, !lut_channel_mask+Y
            or A, <r3
            mov <r3, A

            mov <global_sfx_endsoonest, Y
            mov <r0, #0
            mov <r0+1, #0
            bra @next_chan

@no_dec:
    ; Compare channel 16-bit tick with minimum (<r0, <r0+1)
    mov A, <global_sfx_tick_counter+1+X
    cmp A, <r0+1
    bcc @new_min
    bne @next_chan
        mov A, <global_sfx_tick_counter+X
        cmp A, <r0
        bcs @next_chan
@new_min:
    mov A, <global_sfx_tick_counter+X
    mov <r0, A
    mov A, <global_sfx_tick_counter+1+X
    mov <r0+1, A
    mov <global_sfx_endsoonest, Y

@next_chan:
    inc X
    inc X
    inc Y
    cmp Y, #8
    bcc @loop

    ; If any channels expired, write KOFF mask to DSP
    mov A, <r3
    beq :+
        mov <REG_DSPADDR, #DSP_KOFF
        mov <REG_DSPDATA, A
    :

    ret

_dir_reset:
    mov A, #0
    mov Y, #0
    :
        mov !global_sampletable+Y, A
        dbnz Y, :-

    ; write hardcoded values for the 63th entry of the sample table
    mov A, #<stream_data
    mov !global_sampletable+252, A
    mov !global_sampletable+254, A
    mov A, #>stream_data
    mov !global_sampletable+253, A
    mov !global_sampletable+255, A ; Set both to the same pointer to simulate a loop

    mov A, #0
    mov Y, #0
    movw <global_nextfree, ya
    movw <global_sfx_end, ya
    movw <global_sample_end, ya
    movw <global_seq_start, ya
    movw <global_seq_end, ya

    mov <REG_APUIO1,#SND_CMD_DIR_RESET

    ret

_lock_sfx_boundary:
    movw ya, <global_nextfree
    movw <global_sfx_end, ya
    mov <REG_APUIO1, #SND_CMD_LOCK_SFX
    ret

_mus_ins_reset:
    ; Stop music playback immediately
    mov <seq_playing, #0

    ; Key off all 8 DSP voices immediately
    mov <REG_DSPADDR, #DSP_KOFF
    mov <REG_DSPDATA, #$FF

    ; Reset sample allocation back to the SFX boundary
    movw ya, <global_sfx_end
    movw <global_nextfree, ya
    mov A, #0
    mov <global_sample_end, A
    mov <global_sample_end+1, A
    mov <global_seq_start, A
    mov <global_seq_start+1, A
    mov <global_seq_end, A
    mov <global_seq_end+1, A

    ; Clear directory entries 0..31 in global_sampletable (32 entries * 4 bytes = 128 bytes)
    mov X, #0
    @clear_dir_loop:
        mov !global_sampletable+X, A
        inc X
        cmp X, #128
        bcc @clear_dir_loop

    ; Clear song instrument tables (slots 0..31: 64 bytes each for tickcounts, rates, adsr)
    mov X, #0
    @clear_ins_tables_loop:
        mov !global_sfx_tickcounts+X, A
        mov !global_sfx_samplerates+X, A
        mov !global_sfx_adsr+X, A
        inc X
        cmp X, #64
        bcc @clear_ins_tables_loop

    ; Clear song instrument tuning table (slots 0..31: 32 bytes)
    mov X, #0
    @clear_tune_loop:
        mov !global_ins_tune+X, A
        inc X
        cmp X, #32
        bcc @clear_tune_loop

    ; Clear all 8 voice counters and reset voice ownership to $FF (unowned)
    mov X, #0
    @clear_voice_loop:
        mov <global_sfx_tick_counter+X, A
        inc X
        cmp X, #16
        bcc @clear_voice_loop

    mov X, #0
    mov A, #$FF
    @clear_voice_owner_loop:
        mov !voice_owner+X, A
        inc X
        cmp X, #8
        bcc @clear_voice_owner_loop

    ; Clear all 8 sequence tracks pointers, loops, return addresses, and overrides
    mov X, #0
    mov A, #0
    @clear_seq_tracks_loop:
        mov <seq_ptr_start+X, A
        mov <seq_ptr+X, A
        mov <seq_ptr_loop+X, A
        mov <seq_track_ret+X, A
        mov !seq_track_adsr_override+X, A
        mov !seq_track_tick_override+X, A
        mov !seq_track_pitch_curr+X, A
        mov !seq_track_pitch_target+X, A
        mov !seq_track_pitch_slide+X, A
        inc X
        cmp X, #16
        bcc @clear_seq_tracks_loop

    ; Reset track wait, channels, instruments, and volumes
    mov X, #0
    @clear_track_bytes_loop:
        mov A, #0
        mov <seq_loop_counter+X, A
        mov <seq_track_wait+X, A
        mov !seq_track_vol_slide_l+X, A
        mov !seq_track_vol_slide_r+X, A
        mov !seq_track_pitch_mode+X, A
        mov !seq_track_ins+X, A
        mov A, #$FF
        mov !seq_track_channel+X, A
        mov A, #31
        mov !seq_track_vol_l+X, A
        mov !seq_track_vol_r+X, A
        inc X
        cmp X, #8
        bcc @clear_track_bytes_loop

    mov <seq_tick_timer, #0
    mov <seq_tick_in_row, #0
    mov <global_sfx_endsoonest, #0

    mov <REG_APUIO1, #SND_CMD_MUS_INS_RESET
    ret

_dsp_reg_write:
    ;(uint8_t addr, uint8_t data)
    mov <REG_DSPADDR,<REG_APUIO2
    mov <REG_DSPDATA,<REG_APUIO3

    mov <REG_APUIO1,#SND_CMD_DSP_SET

    ret

; Call to revert the SPC to the initial IPL loader.
_reset_spc:
    ; Reply to the main CPU
    mov <REG_APUIO1,#SND_CMD_SOFTRESET
    
    ; Mute all master and echo volumes
    mov A, #$00

    mov <REG_DSPADDR,#DSP_MVOL0L
    mov <REG_DSPDATA, A

    mov <REG_DSPADDR,#DSP_MVOL0R
    mov <REG_DSPDATA, A

    mov <REG_DSPADDR,#DSP_EVOLL
    mov <REG_DSPDATA, A

    mov <REG_DSPADDR,#DSP_EVOLR
    mov <REG_DSPDATA, A

    ; Reset control register and re-enable boot IPL
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
        mov <mul16_fix, #$00
        mov <mul16_fix+1, #$01
        addw ya, <mul16_fix
    :
    addw ya, <mul16_scratch
    movw <mul16_product_32+2,ya

    ret

;_div_16_by_abs3:
    ; divide 16 bit number by 3 by multiplying by 1/3
    ; enter with
    ; A containing the hi byte of the number to be divided by 3
    ; Y containing the lo byte of the number to be divided by 3
    ; the hi byte of the partial product is kept in A or saved
    ; on the stack when neccessary
    ; the product (N/3 quotient) is returned hi byte in A,
    ; lo byte in Y
    ; save the number in lo_temp, hi_temp
    ;mov Y, <divabs3_dividend
    ;mov A, <divabs3_dividend+1

    ;mov <divabs3_lo_temp, Y
    ;mov <divabs3_lo_product, Y
    ;mov <divabs3_hi_temp, A

    ;mov Y, #$09
    ;clrc
    ;bcc ENTER

    ; each pass through loop adds the number in
    ; lo_temp, hi_temp to the partial product and
    ; then divides the partial product by 4
    ;LOOP:
    ;push A
    ;mov A, <divabs3_lo_product
    ;adc A, <divabs3_lo_temp
    ;mov <divabs3_lo_product, A
    ;pop A
    ;adc A, <divabs3_hi_temp
    ;ENTER:
    ;ror A
    ;ror <divabs3_lo_product
    ;lsr A
    ;ror <divabs3_lo_product
    ;dec Y
    ;bne LOOP
    ;mov Y, <divabs3_lo_product

    ;mov <divabs3_quotient, Y
    ;mov <divabs3_quotient+1, A
    ;movw ya, <divabs3_quotient

    ; Results in above variable, also in YA

    ;ret

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
