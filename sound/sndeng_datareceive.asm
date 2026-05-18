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
    mov <REG_APUIO1,<REG_APUIO1

    ; Calculate where the pointers should be
    mov <r8, #0
    mov <r8+1, #0

    movw ya, <global_sample_end
    cmpw ya, <r8
    bne :+
        movw ya, <global_nextfree
        movw <global_sample_end, ya
    :

    mov A, <r1
    asl A
    mov X, A

    movw ya, <global_nextfree
    movw <global_sample_end, ya
    movw <r2, ya

    movw ya, <global_nextfree
    clrc
    addw ya,<r0
    movw <global_nextfree,ya

    mov A,#<global_sampledata
    mov Y,#>global_sampledata
    clrc
    addw ya,<r2
    movw <r2,ya ; save the starting pointer

    mov <seq_ptr+X, A
    mov <seq_ptr+1+X, Y
    mov <seq_ptr_start+X, A
    mov <seq_ptr_start+1+X, Y

    call !_data_upload_loop

    ret
    

_sfx_upload:
    ;(uint8_t id, uint16_t len)
    ; length come first
    mov Y,<REG_APUIO3
    mov A,<REG_APUIO2

    movw <r0,ya

    mov <REG_APUIO1,<REG_APUIO1 ;echo the opcode.

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_DATA_UPLOAD_SLOT
    :
        cbne <REG_APUIO1, :-
        ;cmp A,<REG_APUIO1
        ;bne :-
    :
        cbne <REG_APUIO1, :-
        ;cmp A,<REG_APUIO1
        ;bne :-

    ; then the tick count and slot
    mov <r6,<REG_APUIO3
    mov <r1,<REG_APUIO2
    mov <r1+1,#0 ; clean the high byte

    mov <REG_APUIO1,<REG_APUIO1

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_DATA_SAMPLE_UPLOAD_SAMPLERATE
    :
        cbne <REG_APUIO1, :-
        ;cmp A,<REG_APUIO1
        ;bne :-
    :
        cbne <REG_APUIO1, :-
        ;cmp A,<REG_APUIO1
        ;bne :-

    ; the sample rate
    mov A, <REG_APUIO2
    mov Y, <REG_APUIO3
    movw <r4,ya

    mov <REG_APUIO1,<REG_APUIO1

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_DATA_SAMPLE_UPLOAD_LOOPSTART
    :
        cbne <REG_APUIO1, :-
        ;cmp A,<REG_APUIO1
        ;bne :-
    :
        cbne <REG_APUIO1, :-
        ;cmp A,<REG_APUIO1
        ;bne :-

    ; loop point
    mov A, <REG_APUIO2
    mov Y, <REG_APUIO3
    movw <r12,ya
        
    mov <REG_APUIO1,<REG_APUIO1

    ; wait for echo from cpu on REG_APUIO1 that indicates that new data has been sent
    mov A,#SND_CMD_DATA_SAMPLE_UPLOAD_ADSR
    :
        cbne <REG_APUIO1, :-
        ;cmp A,<REG_APUIO1
        ;bne :-
    :
        cbne <REG_APUIO1, :-
        ;cmp A,<REG_APUIO1
        ;bne :-

    ; copy the ADSR settings
    mov A, <REG_APUIO2
    mov Y, <REG_APUIO3
    movw <r10,ya

    ; Sample can be copied to RAM, let the other side write out now since it'll be a while
    mov <REG_APUIO1,<REG_APUIO1

    ; Calculate where the pointers should be
    ; for SFX, both pointers should be identical
    mov <r2, <global_nextfree
    mov <r2+1, <global_nextfree+1

    mov A,#<global_sampledata
    mov Y,#>global_sampledata

    clrc

    addw ya,<r2
    movw <r2,ya ; save the starting pointer

    ; Check if overflow
    mov <r3, #$00
    mov <r3+1, #$e0 ; 56KB
    
    movw ya,<global_nextfree ; current size
    clrc
    addw ya,<r0 ; length
    movw <global_nextfree,ya

    ; Write the tick count of the current sample
    ; it's in <r6
    mov A,#<global_sfx_tickcounts
    mov Y,#>global_sfx_tickcounts
    clrc
    addw ya,<r1
    movw <r7,ya ; now points to sample tick length table

    mov Y, #0
    mov A, <r6
    mov [<r7]+y,a

    ; Write the slot the sample belongs to
    asl <r1
    rol <r1+1
    mov <r5, <r1
    mov <r5+1, <r1+1 ; for sample rate table
    asl <r1
    rol <r1+1 ; mul 4, for sample pointer table

    ; Write new values to sample pointer table
    mov A,#<global_sampletable
    mov Y,#>global_sampletable
    clrc
    addw ya,<r1
    movw <r1,ya ; now points to sample pointer table

    mov Y, #0
    mov A, <r2
    mov X, <r2+1
    mov [<r1]+y,a
    mov A,X
    mov Y, #1
    mov [<r1]+y,a

    mov A, <r2
    mov Y, <r2+1
    clrc
    addw ya,<r12 ; add the loop point offset
    movw <r13,ya

    mov A, <r13
    mov X, <r13+1

    mov Y, #2
    mov [<r1]+y,a
    mov A,X
    mov Y, #3
    mov [<r1]+y,a

    ; Write new values to sample rate table
    ; copy it again to another variable
    mov <r11,<r5
    mov <r11+1,<r5+1

    mov A,#<global_sfx_samplerates
    mov Y,#>global_sfx_samplerates
    clrc
    addw ya,<r5
    movw <r5,ya ; now points to sample rate table

    mov Y, #0
    mov A, <r4
    mov X, <r4+1

    mov [<r5]+y,a
    inc Y
    mov A,X
    mov [<r5]+y,a

    ; finally ADSR
    ; <r10
    mov A,#<global_sfx_adsr
    mov Y,#>global_sfx_adsr
    clrc
    addw ya,<r11
    movw <r11,ya ; now points to ADSR table

    mov Y, #0
    mov A, <r10
    mov X, <r10+1

    mov [<r11]+y,a
    inc Y
    mov A,X
    mov [<r11]+y,a

    ;call !_data_upload_loop
    call !_data_upload_loop_2byte

    ret

_data_upload_loop:
    ; Begin copy
    ; r2 contains the pointer to the write dest

    ; Copy r2 to @abs_ptr
    mov A, <r2
    mov !@abs_ptr+1, A
    mov A, <r2+1
    mov !@abs_ptr+2, A

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
        mov <REG_APUIO0,Y
        ;mov [<r2]+Y,A ; Replace with abs below
        @abs_ptr: 
        mov !$0000+Y,A
        inc y
        bne @loop
            ;inc <r2+1 ; ditto
            inc !@abs_ptr+2
        @check_end:
        bpl @loop
        cmp Y,<REG_APUIO0
        bpl @loop

    ret

_data_upload_loop_2byte:
    ; Begin copy
    ; r2 contains the pointer to the write dest

    ; Copy r2 to @abs_ptr_0 and @abs_ptr_1 (byte 0 and 1)
    mov A, <r2
    mov !@abs_ptr_0+1, A
    mov !@abs_ptr_1+1, A
    mov A, <r2+1
    mov !@abs_ptr_0+2, A
    mov !@abs_ptr_1+2, A

    ; increment abs_ptr_1
    inc !@abs_ptr_1+1
    bne :+
        inc !@abs_ptr_1+2
    :

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
        inc y
        bne @loop
            inc !@abs_ptr_0+2
            inc !@abs_ptr_1+2
        @check_end:
        bpl @loop
        cmp Y,<REG_APUIO0
        bpl @loop

    ret
