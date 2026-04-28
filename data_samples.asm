; For VBCC

    section "_rodata.far.bindata.0"
    global _data_snd_smp_sfx_whoosh
_data_snd_smp_sfx_whoosh:
    incbin "sound/sfx/sfx_whoosh.brr"
    global _data_snd_smp_sfx_punch
_data_snd_smp_sfx_punch:
    incbin "sound/sfx/sfx_punch.brr"
    global _data_snd_smp_sfx_footstep
_data_snd_smp_sfx_footstep:
    incbin "sound/sfx/sfx_footstep.brr"
    global _data_snd_smp_sfx_coin
_data_snd_smp_sfx_coin:
    incbin "sound/sfx/sfx_coin.brr"
    global _data_snd_smp_sfx_msgclick
_data_snd_smp_sfx_msgclick:
    incbin "sound/sfx/sfx_msgclick.brr"
    global _data_snd_smp_sfx_switch
_data_snd_smp_sfx_switch:
    incbin "sound/sfx/sfx_switch.brr"
    global _data_snd_smp_sfx_splash
_data_snd_smp_sfx_splash:
    incbin "sound/sfx/sfx_splash.brr"
    global _data_snd_smp_sfx_splathit
_data_snd_smp_sfx_splathit:
    incbin "sound/sfx/sfx_splathit.brr"
    global _data_snd_smp_sfx_flamestream
_data_snd_smp_sfx_flamestream:
    incbin "sound/sfx/sfx_flamestream.brr"
    global _data_snd_smp_sfx_firecrackle
_data_snd_smp_sfx_firecrackle:
    incbin "sound/sfx/sfx_firecrackle.brr"
    global _data_snd_smp_sfx_bounce
_data_snd_smp_sfx_bounce:
    incbin "sound/sfx/sfx_bounce.brr"

; START OF INSTRUMENT SAMPLES

; STANDARD INSTRUMENTS
    global _data_snd_smp_ins_tone_square
_data_snd_smp_ins_tone_square:
    incbin "sound/ins/synth/ins_tone_square_b4.brr"

    global _data_snd_smp_ins_bass
_data_snd_smp_ins_bass:
    incbin "sound/ins/bass/ins_bass_c2.brr"
    
    global _data_snd_smp_ins_piano
_data_snd_smp_ins_piano:
    incbin "sound/ins/piano/ins_piano_g3.brr"

    global _data_snd_smp_ins_guitar_acos
_data_snd_smp_ins_guitar_acos:
    incbin "sound/ins/guitar/ins_guitar_acos_g3.brr"
    global _data_snd_smp_ins_guitar_dist
_data_snd_smp_ins_guitar_dist:
    incbin "sound/ins/guitar/ins_guitar_dist_c3.brr"

    global _data_snd_smp_ins_flute
_data_snd_smp_ins_flute:
    incbin "sound/ins/wind/ins_flute_c4.brr"
    global _data_snd_smp_ins_sax
_data_snd_smp_ins_sax:
    incbin "sound/ins/wind/ins_sax_fs4.brr"
    global _data_snd_smp_ins_trumpet
_data_snd_smp_ins_trumpet:
    incbin "sound/ins/wind/ins_trumpet_b4.brr"

    global _data_snd_smp_ins_strings
_data_snd_smp_ins_strings:
    incbin "sound/ins/string/ins_stringensemble_c5.brr"
    global _data_snd_smp_ins_cello
_data_snd_smp_ins_cello:
    incbin "sound/ins/string/ins_cello_c3.brr"
    global _data_snd_smp_ins_violin
_data_snd_smp_ins_violin:
    incbin "sound/ins/string/ins_violin_c5.brr"

    global _data_snd_smp_ins_marimba
_data_snd_smp_ins_marimba:
    incbin "sound/ins/other/ins_marimba_c3.brr"

; DRUMS/ONE-SHOT
    global _data_snd_smp_ins_drum_kick
_data_snd_smp_ins_drum_kick:
    incbin "sound/ins/ins_drum_kick.brr"

    global _data_snd_smp_ins_drum_snare
_data_snd_smp_ins_drum_snare:
    incbin "sound/ins/ins_drum_snare.brr"

    global _data_snd_smp_ins_drum_hihat
_data_snd_smp_ins_drum_hihat:
    incbin "sound/ins/ins_drum_hihat.brr"

    global _data_snd_smp_ins_drum_cymbals
_data_snd_smp_ins_drum_cymbals:
    incbin "sound/ins/ins_drum_cymbals.brr"

;    global _data_snd_smp_ins_drum_tom
;_data_snd_smp_ins_drum_tom:
;    incbin "sound/ins/ins_drum_tom.brr"

    global _data_snd_smp_ins_drum_clap
_data_snd_smp_ins_drum_clap:
    incbin "sound/ins/ins_drum_clap.brr"

    global _data_snd_smp_ins_drum_stick
_data_snd_smp_ins_drum_stick:
    incbin "sound/ins/ins_drum_stick.brr"