.segment "SPCIMAGE"

_mus_start:
    mov <seq_playing, #1

    mov <REG_APUIO1,#SND_CMD_MUS_START

    ret

_mus_pause:
    mov <seq_playing, #0

    mov <REG_APUIO1,#SND_CMD_MUS_PAUSE

    ret

_mus_stop:
    mov <seq_playing, #0
    mov <seq_tick_timer, #0
    mov <seq_process_track, #0

    mov X, #0
    @loop_16:
        mov A, <seq_ptr_start+X
        mov <seq_ptr+X, A
        mov A, <seq_ptr_start+1+X
        mov <seq_ptr+1+X, A

        mov A, #0
        mov <seq_ptr_loop+X, A
        mov <seq_ptr_loop+1+X, A
        mov <global_sfx_tick_counter+X, A
        mov <global_sfx_tick_counter+1+X, A

        mov !seq_track_adsr_override+X, A
        mov !seq_track_adsr_override+1+X, A
        mov !seq_track_tick_override+X, A
        mov !seq_track_tick_override+1+X, A

        inc X
        inc X
        cmp X, #16
        bcc @loop_16

    mov X, #0
    @loop_8:
        mov A, #0
        mov <seq_loop_counter+X, A
        mov <seq_track_wait+X, A
        mov !seq_track_ins+X, A
        mov !seq_track_vol_l+X, A
        mov !seq_track_vol_r+X, A
        mov A, #$ff
        mov !seq_track_channel+X, A
        mov !voice_owner+X, A
        inc X
        cmp X, #8
        bcc @loop_8

    ; Silence all voices and clear tick counters
    mov <REG_DSPADDR, #DSP_KOFF
    mov <REG_DSPDATA, #$ff

    ; Update channel LRUs to reflect all voices free
    mov A, #0
    call !_update_channel_lru

    mov <REG_DSPADDR, #DSP_KOFF
    mov <REG_DSPDATA, #$00
    
    mov <seq_speed, #6
    mov <seq_tick_in_row, #0

    mov <REG_APUIO1,#SND_CMD_MUS_STOP

    ret

; Calculate the correct tempo from main CPU first
_mus_set_tempo:
    mov <seq_tick_timer_target, <REG_APUIO3
    mov <REG_T1DIV, <REG_APUIO2
    
    mov <REG_APUIO1,#SND_CMD_MUS_SET_TEMPO
    
    ret

_mus_set_speed:
    mov A, <REG_APUIO2
    bne :+
        mov A, #6
    :
    mov <seq_speed, A
    mov <seq_tick_in_row, #0

    mov <REG_APUIO1,#SND_CMD_MUS_SET_SPEED

    ret

_mus_set_outputmode:
    mov A, <REG_APUIO2
    mov !seq_force_mono, A
    
    mov <REG_APUIO1,#SND_CMD_MUS_SET_OUTPUTMODE
    
    ret

_set_music_volume:
    mov A, <REG_APUIO2
    mov !seq_music_volume, A
    mov <REG_APUIO1, #SND_CMD_SET_MUSIC_VOL
    ret

_set_sfx_volume:
    mov A, <REG_APUIO2
    mov !seq_sfx_volume, A
    mov <REG_APUIO1, #SND_CMD_SET_SFX_VOL
    ret

_set_voice_volume:
    mov A, <REG_APUIO2
    mov !seq_voice_volume, A
    mov <REG_APUIO1, #SND_CMD_SET_VOICE_VOL
    ret
    