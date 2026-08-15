_mus_seq_upload:
    ;(uint8_t track, uint16_t len)
    ; length come first
    mov Y,<REG_APUIO3
    mov A,<REG_APUIO2

    movw <r0,ya

    mov <REG_APUIO1,<REG_APUIO1 ;echo the opcode.

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_SEQ_UPLOAD_TRACK
    :
        cbne <REG_APUIO1, :-
        ;cmp A,<REG_APUIO1
        ;bne :-
    :
        cbne <REG_APUIO1, :-
        ;cmp A,<REG_APUIO1
        ;bne :-

    mov <r1,<REG_APUIO2

    mov <REG_APUIO1,<REG_APUIO1

    ; Sequence can be copied to RAM, let the other side write out now since it'll be a while
    mov <REG_APUIO0, #$ff ; make sure the other side doesn't get Y == 0
    mov <REG_APUIO1,<REG_APUIO1

    ; Calculate where the pointers should be
    mov A, <global_sample_end
    or A, <global_sample_end+1
    bne :+
        ; First sequence upload after samples: lock in sample boundary and sequence start
        movw ya, <global_nextfree
        movw <global_sample_end, ya
        movw <global_seq_start, ya
    :

    ; If uploading Track 0, reset sequence allocation back to sequence start
    ; and clear all track starting pointers so unused tracks from a previous song are disabled
    mov A, <r1
    bne @not_track_0
        movw ya, <global_seq_start
        movw <global_nextfree, ya

        mov X, #0
        mov A, #0
        @clear_track_ptrs:
            mov <seq_ptr_start+X, A
            mov <seq_ptr+X, A
            inc X
            cmp X, #16
            bcc @clear_track_ptrs
    @not_track_0:

    mov A, <r1
    asl A
    mov X, A

    movw ya, <global_nextfree
    movw <r2, ya

    movw ya, <global_nextfree
    clrc
    addw ya,<r0
    bcc :+
        ret
    :
    movw <global_nextfree,ya
    movw <global_seq_end,ya

    mov A,#<global_sampledata
    mov Y,#>global_sampledata
    clrc
    addw ya,<r2
    bcc :+
        ret
    :
    movw <r2,ya ; save the starting pointer

    mov <seq_ptr+X, A
    mov <seq_ptr+1+X, Y
    mov <seq_ptr_start+X, A
    mov <seq_ptr_start+1+X, Y

    ;call !_data_upload_loop
    call !_data_upload_loop_2byte

    ret
    

_sfx_upload:
    ;(uint8_t id, uint16_t len)
    ; length come first
    mov Y,<REG_APUIO3
    mov A,<REG_APUIO2

    movw <r0,ya

    mov <REG_APUIO1,<REG_APUIO1 ;echo the opcode.

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_DATA_SAMPLE_UPLOAD_TICK
    :
        cbne <REG_APUIO1, :-
    :
        cbne <REG_APUIO1, :-

    ; then the tick count
    mov <r6,<REG_APUIO2
    mov <r6+1,<REG_APUIO3

    mov <REG_APUIO1,<REG_APUIO1

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_DATA_SAMPLE_UPLOAD_SLOT
    :
        cbne <REG_APUIO1, :-
    :
        cbne <REG_APUIO1, :-

    ; then the slot
    mov <r1,<REG_APUIO2
    mov <r1+1,#0 ; clean the high byte

    mov <REG_APUIO1,<REG_APUIO1

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_DATA_SAMPLE_UPLOAD_SAMPLERATE
    :
        cbne <REG_APUIO1, :-
    :
        cbne <REG_APUIO1, :-

    ; the sample rate
    mov A, <REG_APUIO2
    mov Y, <REG_APUIO3
    movw <r4,ya

    mov <REG_APUIO1,<REG_APUIO1

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_DATA_SAMPLE_UPLOAD_LOOPSTART
    :
        cbne <REG_APUIO1, :-
    :
        cbne <REG_APUIO1, :-

    ; loop point
    mov A, <REG_APUIO2
    mov Y, <REG_APUIO3
    movw <r12,ya
        
    mov <REG_APUIO1,<REG_APUIO1

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_DATA_SAMPLE_UPLOAD_ADSR
    :
        cbne <REG_APUIO1, :-
    :
        cbne <REG_APUIO1, :-

    ; copy the ADSR settings
    mov A, <REG_APUIO2
    mov Y, <REG_APUIO3
    movw <r10,ya

    ; Sample can be copied to RAM, let the other side write out now since it'll be a while
    mov <REG_APUIO0, #$ff ; make sure the other side doesn't get Y == 0
    mov <REG_APUIO1,<REG_APUIO1

    ; Calculate where the pointers should be
    ; for SFX, both pointers should be identical
    movw ya, <global_nextfree
    movw <r2, ya

    mov A,#<global_sampledata
    mov Y,#>global_sampledata
    clrc
    addw ya,<r2
    bcc :+
        ; Start address + offset wraps past $FFFF
        bra @reject_upload
    :
    movw <r2,ya ; save the starting pointer

    movw ya,<global_nextfree ; current size
    clrc
    addw ya,<r0 ; length
    bcc :+
    @reject_upload:
        ; TODO: this is broken
        ret
    :
    movw <global_nextfree,ya

    ; Write slot tables using direct absolute indexing
    ; X = slot * 2
    mov A, <r1
    asl A
    mov X, A

    ; Write tick count (2 bytes per slot)
    mov A, <r6
    mov !global_sfx_tickcounts+X, A
    mov A, <r6+1
    mov !global_sfx_tickcounts+1+X, A

    ; Write sample rate (2 bytes per slot)
    mov A, <r4
    mov !global_sfx_samplerates+X, A
    mov A, <r4+1
    mov !global_sfx_samplerates+1+X, A

    ; Write ADSR (2 bytes per slot)
    mov A, <r10
    mov !global_sfx_adsr+X, A
    mov A, <r10+1
    mov !global_sfx_adsr+1+X, A

    ; X = slot * 4 for sample directory table (4 bytes per entry)
    mov A, <r1
    asl A
    asl A
    mov X, A

    ; Write sample start pointer
    mov A, <r2
    mov !global_sampletable+X, A
    mov A, <r2+1
    mov !global_sampletable+1+X, A

    ; Write loop start pointer (start + loop offset)
    movw ya, <r2
    clrc
    addw ya, <r12
    mov !global_sampletable+2+X, A
    mov A, Y
    mov !global_sampletable+3+X, A

    call !_data_upload_loop_2byte

    ret

_stream_upload:
    ; fixed location, so easier
    ; assumes fixed size
    ;mov <r0, #72 ; always 72 bytes
    ;mov <r0+1, #0

    ;mov <r2, #<stream_data
    ;mov <r2+1, #>stream_data

    mov <REG_APUIO1,<REG_APUIO1 ;echo the opcode.

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_STREAM_UPLOAD
    :
        cbne <REG_APUIO1, :-
    :
        cbne <REG_APUIO1, :-

    ; Sample can be copied to RAM, let the other side write out now since it'll be a while
    mov <REG_APUIO0, #$ff ; make sure the other side doesn't get Y == 0

    call !_data_upload_loop_stream

    mov <stream_watchdog, #0
    mov <stream_watchdog+1, #6 ; 1536 ticks
    
    mov A, <stream_active
    beq @preload_buf_wait ; Avoid playing the stream while it's not filled
        cmp A, #1
        beq @preload_buf_wait
        cmp A, #3
        bcs @stream_playing_already
            call !_stream_play
    @preload_buf_wait:
    inc <stream_active
    @stream_playing_already:
    
    ret

;_data_upload_loop:
;    ; Begin copy
;    ; r2 contains the pointer to the write dest
;
;    ; Copy r2 to @abs_ptr
;    mov A, <r2
;    mov !@abs_ptr+1, A
;    mov A, <r2+1
;    mov !@abs_ptr+2, A
;
;    mov Y,#0
;
;    @startup:
;        cmp Y,<REG_APUIO0
;        bne @startup
;        bra @write
;    @loop:
;        cmp Y,<REG_APUIO0
;        bne @check_end
;
;        @write:
;        mov A,<REG_APUIO1
;        mov <REG_APUIO0,Y
;        @abs_ptr: 
;        mov !$0000+Y,A
;        inc y
;        bne @loop
;            inc !@abs_ptr+2
;        @check_end:
;        bpl @loop
;        cmp Y,<REG_APUIO0
;        bpl @loop

;    ret

_data_upload_loop_2byte:
    ; Begin copy
    ; r0 contains the length of data transfer (must be even)
    ; r2 contains the pointer to the write dest

    ; halve the length
    lsr <r0+1
    ror <r0

    ; Copy r2 to @abs_ptr_0 and @abs_ptr_1 (byte 0 and 1)
    mov A, <r2
    mov !@abs_ptr_0+1, A

    mov A, <r2+1
    mov !@abs_ptr_0+2, A

    ; Calculate offsetted second pointer
    movw ya, <r2
    clrc
    addw ya, <r0
    mov !@abs_ptr_1+1, A
    mov !@abs_ptr_1+2, Y

    mov Y,#0

    @startup:
        cmp Y,<REG_APUIO0
        bne @startup
        bra @write
    @loop:
        cmp Y,<REG_APUIO0
        bne @check_end

        @write:
        mov A,<REG_APUIO1
        @abs_ptr_0: 
        mov !$0000+Y,A
        mov A,<REG_APUIO2
        mov <REG_APUIO0,Y
        @abs_ptr_1: 
        mov !$0000+Y,A
        inc y
        bne @loop
            inc !@abs_ptr_0+2
            inc !@abs_ptr_1+2
        @check_end:
        bpl @loop
        cmp Y,<REG_APUIO0
        bpl @loop

    ret

lut_stream_offsets:
    .word stream_data + 0
    .word stream_data + 72
    .word stream_data + 144
    .word stream_data + 216

_data_upload_loop_stream:
    mov A, <stream_current_block
    asl A
    mov X, A

    mov A, !lut_stream_offsets+X
    mov !@abs_ptr_0+1, A
    clrc
    adc A, #36
    mov !@abs_ptr_1+1, A

    mov A, !lut_stream_offsets+1+X
    mov !@abs_ptr_0+2, A
    adc A, #0
    mov !@abs_ptr_1+2, A

    mov Y, #0
@startup:
    cmp Y, <REG_APUIO0
    bne @startup
    bra @write
@loop:
    cmp Y, <REG_APUIO0
    bne @check_end

@write:
    mov A, <REG_APUIO1
@abs_ptr_0:
    mov !$0000+Y, A
    mov A, <REG_APUIO2
    mov <REG_APUIO0, Y
@abs_ptr_1:
    mov !$0000+Y, A
    inc y
    bra @loop
@check_end:
    bpl @loop
    cmp Y, <REG_APUIO0
    bpl @loop

    mov A, <stream_current_block
    inc A
    and A, #$03
    mov <stream_current_block, A

    ret

;_data_upload_loop_3byte:
;    ; Begin copy
;    ; r0 contains the length of data transfer
;    ; r2 contains the pointer to the write dest
;
;    ; first calculate the length divided
;    mov <divabs3_dividend, <r0
;    mov <divabs3_dividend+1, <r0+1
;
;    ; Call div by 3
;    call !_div_16_by_abs3
;
;    ; Save chunk length to r0
;    mov <r0, <divabs3_quotient
;    mov <r0+1, <divabs3_quotient+1
;
;    ; Copy r2 to @abs_ptr_0-2 (byte 0-2)
;    mov A, <r2
;    mov !@abs_ptr_0+1, A
;
;    mov A, <r2+1
;    mov !@abs_ptr_0+2, A
;
;    ; Calculate offsetted second and third pointer
;    movw ya, <r2
;    clrc
;    addw ya, <r0
;    mov !@abs_ptr_1+1, A
;    mov !@abs_ptr_1+2, Y
;    addw ya, <r0
;    mov !@abs_ptr_2+1, A
;    mov !@abs_ptr_2+2, Y
;
;    mov Y,#0
;
;    @startup:
;        cmp Y,<REG_APUIO0
;        bne @startup
;        bra @write
;    @loop:
;        cmp Y,<REG_APUIO0
;        bne @check_end
;
;        @write:
;        mov A,<REG_APUIO1
;        @abs_ptr_0: 
;        mov !$0000+Y,A
;        mov A,<REG_APUIO2
;        @abs_ptr_1: 
;        mov !$0000+Y,A
;        mov A,<REG_APUIO3
;        mov <REG_APUIO0,Y
;        @abs_ptr_2: 
;        mov !$0000+Y,A
;        inc y
;        bne @loop
;            inc !@abs_ptr_0+2
;            inc !@abs_ptr_1+2
;            inc !@abs_ptr_2+2
;        @check_end:
;        bpl @loop
;        cmp Y,<REG_APUIO0
;        bpl @loop
;
;    ret
