.segment "SPCZEROPAGE"

; scratch/temp variables, first group. pass parameters in A, X, or r8-r15
r0: .res 2
r1: .res 2

; r2-r7 are also DSP parameters. Use these to make code easier to read.
; _start_note will read from these zp vars
r2: .res 2
dsp_param_vol_l = r2

r3: .res 2
dsp_param_vol_r = r3

r4: .res 2
dsp_param_vpitch = r4

r5: .res 2
dsp_param_srcn = r5

r6: .res 2
dsp_param_adsr = r6 ; no reservation for GAIN as it is left at $7f permanently

r7: .res 2
dsp_param_kon = r7

; scratch/temp variables, second group
; these can also be thought as the parameter list past the first two bytes
r8: .res 2
r9: .res 2
r10: .res 2
r11: .res 2
r12: .res 2
r13: .res 2
r14: .res 2
r15: .res 2

global_last_cmd: .res 1
global_received_cmd: .res 1
global_tick_ran: .res 1

global_sfx_endsoonest: .res 1

global_sfx_queue_counter: .res 1
global_sfx_queue: .res 8
global_sfx_tick_counter: .res 8

global_nextfree: .res 2
global_sample_end: .res 2

global_current_command_counter: .res 1

seq_playing: .res 1

seq_current_track: .res 1
seq_current_track_ptr: .res 2

seq_tick_timer: .res 1
seq_tick_timer_target: .res 1

seq_ptr_start: .res (2 * 8) ; starting pointer
seq_ptr: .res (2 * 8) ; return pointer

; loop counters for each sequence track. when non-zero and encountering a loop opcode, the track goes to the loop point and the counter is decremented.
seq_ptr_loop: .res (2 * 8) ; loop ptr
seq_loop_counter: .res 8 
seq_track_wait: .res 8 ; if non-zero, nops this row. Placed here to make clearing values more convenient

seq_note_prefix: .res 1
seq_note_adsr_override: .res 2
seq_note_tick_override: .res 1

mul16_a: .res 2
mul16_b: .res 2
mul16_product_32: .res 4
mul16_scratch: .res 2
mul16_fix: .res 2

divabs3_dividend: .res 2
divabs3_lo_temp: .res 1
divabs3_lo_product: .res 1
divabs3_hi_temp: .res 1
divabs3_quotient: .res 2

stream_active: .res 1

.segment "SPCBSS"

.align $100
BSS_START:
global_sampledata: .res (60 * 1024) ; 60 KB
.align $100
INTERNAL_UNIQUE_SAMPLE_COUNT = 64
global_sampletable: .res (4 * INTERNAL_UNIQUE_SAMPLE_COUNT)

global_sfx_samplerates: .res (2 * INTERNAL_UNIQUE_SAMPLE_COUNT)
global_sfx_adsr: .res (2 * INTERNAL_UNIQUE_SAMPLE_COUNT)
global_sfx_tickcounts: .res INTERNAL_UNIQUE_SAMPLE_COUNT
global_ins_tune: .res INTERNAL_UNIQUE_SAMPLE_COUNT

; for sample streaming
stream_data: .res 72

