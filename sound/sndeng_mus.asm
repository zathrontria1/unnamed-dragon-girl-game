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

    mov X, #0
    @loop:
        mov A, <seq_ptr_start+X
        mov <seq_ptr+X, A
        mov A, <seq_ptr_start+1+X
        mov <seq_ptr+1+X, A

        mov A, #0
        mov <seq_ptr_loop+X, A
        mov <seq_ptr_loop+1+X, A
        mov <seq_loop_counter+X, A
        mov <seq_loop_counter+1+X, A

        inc X
        inc X
        cmp X, #16
        bcc @loop
    
    mov <REG_APUIO1,#SND_CMD_MUS_STOP

    ret

; Calculate the correct tempo from main CPU first
_mus_set_tempo:
    mov <seq_tick_timer_target, <REG_APUIO3
    mov <REG_T1DIV, <REG_APUIO2
    
    mov <REG_APUIO1,#SND_CMD_MUS_SET_TEMPO
    
    ret