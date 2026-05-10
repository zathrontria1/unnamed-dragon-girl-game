 zpage r0
 zpage r1
 zpage r2
 zpage r3
 global ___exit
 global ___start
 section start_text.startup
___start:
 clc
 xce
 rep #$30
 a16
 x16

 lda #___stackend
 tcs
 lda #r0
 and #$ff00
 tcd
 pea #^__NDS
 plb

 sep #$20
 a8

 pla

 ; Write the MVN and RTL opcode
 lda #$54
 sta r6

 lda #$6b
 sta r7+1
 
 ; Near bank WRAM clear
 ; Write addresses and length of the clear area
 lda #^__NBS
 sta r6+1
 sta r7 ; Here, source and dest banks are the same.

 rep #$30 ; axy16
 a16
 x16
 
 lda #0
 sta >__NBS ; Write 0 to start bytes
 
 lda #<__NBE
 sec
 sbc #<__NBS+2
 bcc .clearfar

 ldx #<__NBS ; Copy source
 ldy #<__NBS+1 ; Copy dest

 jsl copy_blockmove
.clearfar:
 ; Repeat for the other banks.
 ; Far bank WRAM clear
 ; Write addresses and length of the clear area
 sep #$20
 a8
 lda #^__FBS
 sta r6+1
 sta r7 ; Here, source and dest banks are the same.

 rep #$30 ; axy16
 a16
 x16
 
 lda #0
 sta >__FBS ; Write 0 to start bytes
 
 lda #<__FBE
 sec
 sbc #<__FBS+2
 bcc .clearhuge

 ldx #<__FBS ; Copy source
 ldy #<__FBS+1 ; Copy dest

 jsl copy_blockmove

 ; Huge bank WRAM clear
.clearhuge:
 sep #$20
 a8
 lda #^__HBS
 sta r6+1
 sta r7 ; Here, source and dest banks are the same.

 rep #$30 ; axy16
 a16
 x16
 
 lda #0
 sta >__HBS ; Write 0 to start bytes
 
 lda #<__HBE
 sec
 sbc #<__HBS+2

 bcc .clear_done

 ldx #<__HBS ; Copy source
 ldy #<__HBS+1 ; Copy dest

 jsl copy_blockmove
.clear_done:

; Begin copying.
; C is the location of the source. S is destination.
 sep #$20
 a8
 lda #^__NDC
 sta r7
 lda #^__NDS
 sta r6+1 ; Source and destination are different. Note that in code the banks are "destination, source"

 rep #$30 ; axy16
 a16
 x16

 lda #<__NDE
 sec
 sbc #<__NDS+1

 bcc .copyfar

 ldx #<__NDC ; Copy source
 ldy #<__NDS ; Copy dest

 jsl copy_blockmove

.copyfar:
 sep #$20
 a8
 lda #^__FDC
 sta r7
 lda #^__FDS
 sta r6+1 ; Source and destination are different. Note that in code the banks are "destination, source"

 rep #$30 ; axy16
 a16
 x16

 lda #<__FDE
 sec
 sbc #<__FDS+1

 bcc .copyhuge

 ldx #<__FDC ; Copy source
 ldy #<__FDS ; Copy dest

 jsl copy_blockmove


.copyhuge:
 sep #$20
 a8
 lda #^__HDC
 sta r7
 lda #^__HDS
 sta r6+1 ; Source and destination are different. Note that in code the banks are "destination, source"

 rep #$30 ; axy16
 a16
 x16

 lda #<__HDE
 sec
 sbc #<__HDS+1

 bcc .copy_done

 ldx #<__HDC ; Copy source
 ldy #<__HDS ; Copy dest

 jsl copy_blockmove

.copy_done:

 ; Clear the stack. Must be manually done without subroutine calls.
 rep #$30
 a16
 x16
 stz ___stack ; clear the first two bytes
 lda #___stacklen-2
 ldx #___stack
 ldy #___stack+1
 mvn #$00, #$00

 ; DB should be at bank $00. Change it to $80 here.
 ; Change data bank to $80
 sep #$20
 a8
 lda #__DBR_init
 pha
 plb

 ; Clear the zero page
 rep #$20
 sep #$10
 a16
 x8
 
 ldx #$0
 
.zp_clear:
 stz $00,x
 inx
 inx
 bne .zp_clear

 rep #$30
 a16
 x16

 lda #0
 txa
 txy

 pha
 pla ; also clear the stack value with 0 for good measure

 jsl ___main
___exit:
 jmp ___exit

copy_blockmove:
 phb
 jsl >r6
 plb
 rtl

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






