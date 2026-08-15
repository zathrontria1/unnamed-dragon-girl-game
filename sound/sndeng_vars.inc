.segment "SPCDIR"
INTERNAL_UNIQUE_SAMPLE_COUNT = 64
global_sampletable: .res (4 * INTERNAL_UNIQUE_SAMPLE_COUNT)

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

global_current_command_counter: .res 1

global_last_cmd: .res 1

global_sfx_endsoonest: .res 1

global_sfx_tick_counter: .res 16 ; eight 16-bit counters

global_nextfree: .res 2
global_sample_end: .res 2
global_seq_start: .res 2
global_seq_end: .res 2

seq_playing: .res 1

seq_current_track: .res 1
seq_current_track_ptr: .res 2
seq_process_track: .res 1

seq_tick_timer: .res 1
seq_tick_timer_target: .res 1

seq_speed: .res 1
seq_tick_in_row: .res 1

seq_ptr_start: .res (2 * 8) ; starting pointer
seq_ptr: .res (2 * 8) ; return pointer

; loop counters for each sequence track. when non-zero and encountering a loop opcode, the track goes to the loop point and the counter is decremented.
seq_ptr_loop: .res (2 * 8) ; loop ptr
seq_loop_counter: .res 8 
seq_track_wait: .res 8 ; if non-zero, nops this row. Placed here to make clearing values more convenient

; Near-future tracker state: Subroutine calls for reusable pattern/phrase macros
seq_track_ret: .res (2 * 8) ; Subroutine return address per track

stream_active: .res 1
stream_current_block: .res 1 ; 0, 1, 2, 3
stream_watchdog: .res 2 ; must be 2 bytes

mul16_a: .res 2
mul16_b: .res 2
mul16_product_32: .res 4
mul16_scratch: .res 2
mul16_fix: .res 2

.segment "SPCSTACKRAM"

; --- Near-future tracker state variables (pitch modulation, portamento, volume slides) ---
; Allocated in lower Page 1 ($0100..$014F); stack operates at $01FF downward.
seq_track_channel: .res 8 ; Currently assigned DSP voice channel (0..7, or $FF if inactive) for real-time modulation
seq_track_vol_slide_l: .res 8 ; Signed 8-bit left volume delta per tick (volume slides / tremolo)
seq_track_vol_slide_r: .res 8 ; Signed 8-bit right volume delta per tick (stereo panning slides)
seq_track_pitch_curr: .res (2 * 8) ; 16-bit current DSP pitch (pitch bends / vibrato)
seq_track_pitch_target: .res (2 * 8) ; 16-bit target DSP pitch for portamento (IT/MOD command G / 3xx)
seq_track_pitch_slide: .res (2 * 8) ; Signed 16-bit pitch delta per tick
seq_track_pitch_mode: .res 8 ; 0 = Off, 1 = Pitch Slide (E/F), 2 = Portamento (G)

.segment "SPCBSS"

BSS_START:

seq_track_ins: .res 8 ; Sticky instrument ID
seq_track_vol_l: .res 8 ; Sticky left volume
seq_track_vol_r: .res 8 ; Sticky right volume
seq_track_adsr_override: .res (2 * 8) ; Sticky ADSR (low/high per track)
seq_track_tick_override: .res (2 * 8) ; Sticky tick duration per track (16-bit)

seq_force_mono: .res 1

seq_music_volume: .res 1
seq_sfx_volume: .res 1
seq_voice_volume: .res 1

global_sfx_samplerates: .res (2 * INTERNAL_UNIQUE_SAMPLE_COUNT)
global_sfx_adsr: .res (2 * INTERNAL_UNIQUE_SAMPLE_COUNT)
global_sfx_tickcounts: .res (2 * INTERNAL_UNIQUE_SAMPLE_COUNT)
global_ins_tune: .res INTERNAL_UNIQUE_SAMPLE_COUNT

; for sample streaming
stream_data: .res 288 ; 72 * 4

; Dynamic sample and sequence memory pool extending to $FFFF
global_sampledata:
