; For Calypsi

    .section cfar,rodata
    .global data_snd_smp_sfx_whoosh
data_snd_smp_sfx_whoosh:
    .incbin "sound/sfx/sfx_whoosh.brr"
    .global data_snd_smp_sfx_punch
data_snd_smp_sfx_punch:
    .incbin "sound/sfx/sfx_punch.brr"
    .global data_snd_smp_sfx_footstep
data_snd_smp_sfx_footstep:
    .incbin "sound/sfx/sfx_footstep.brr"
    .global data_snd_smp_sfx_coin
data_snd_smp_sfx_coin:
    .incbin "sound/sfx/sfx_coin.brr"
    .global data_snd_smp_sfx_msgclick
data_snd_smp_sfx_msgclick:
    .incbin "sound/sfx/sfx_msgclick.brr"
    .global data_snd_smp_sfx_switch
data_snd_smp_sfx_switch:
    .incbin "sound/sfx/sfx_switch.brr"
    .global data_snd_smp_sfx_splash
data_snd_smp_sfx_splash:
    .incbin "sound/sfx/sfx_splash.brr"
    .global data_snd_smp_sfx_splathit
data_snd_smp_sfx_splathit:
    .incbin "sound/sfx/sfx_splathit.brr"
    .global data_snd_smp_sfx_flamestream
data_snd_smp_sfx_flamestream:
    .incbin "sound/sfx/sfx_flamestream.brr"
    .global data_snd_smp_sfx_firecrackle
data_snd_smp_sfx_firecrackle:
    .incbin "sound/sfx/sfx_firecrackle.brr"
    .global data_snd_smp_sfx_bounce
data_snd_smp_sfx_bounce:
    .incbin "sound/sfx/sfx_bounce.brr"

; START OF INSTRUMENT SAMPLES

; STANDARD INSTRUMENTS
    .global data_snd_smp_ins_tone_square
data_snd_smp_ins_tone_square:
    .incbin "sound/ins/synth/ins_tone_square_b4.brr"

    .global data_snd_smp_ins_bass
data_snd_smp_ins_bass:
    .incbin "sound/ins/bass/ins_bass_c2.brr"
    
    .global data_snd_smp_ins_piano
data_snd_smp_ins_piano:
    .incbin "sound/ins/piano/ins_piano_g3.brr"

    .global data_snd_smp_ins_guitar_acos
data_snd_smp_ins_guitar_acos:
    .incbin "sound/ins/guitar/ins_guitar_acos_g3.brr"
    .global data_snd_smp_ins_guitar_dist
data_snd_smp_ins_guitar_dist:
    .incbin "sound/ins/guitar/ins_guitar_dist_c3.brr"

    .global data_snd_smp_ins_flute
data_snd_smp_ins_flute:
    .incbin "sound/ins/wind/ins_flute_c4.brr"
    .global data_snd_smp_ins_sax
data_snd_smp_ins_sax:
    .incbin "sound/ins/wind/ins_sax_fs4.brr"
    .global data_snd_smp_ins_trumpet
data_snd_smp_ins_trumpet:
    .incbin "sound/ins/wind/ins_trumpet_b4.brr"

    .global data_snd_smp_ins_strings
data_snd_smp_ins_strings:
    .incbin "sound/ins/string/ins_stringensemble_c5.brr"
    .global data_snd_smp_ins_cello
data_snd_smp_ins_cello:
    .incbin "sound/ins/string/ins_cello_c3.brr"
    .global data_snd_smp_ins_violin
data_snd_smp_ins_violin:
    .incbin "sound/ins/string/ins_violin_c5.brr"

    .global data_snd_smp_ins_marimba
data_snd_smp_ins_marimba:
    .incbin "sound/ins/other/ins_marimba_c3.brr"

; DRUMS/ONE-SHOT
    .global data_snd_smp_ins_drum_kick
data_snd_smp_ins_drum_kick:
    .incbin "sound/ins/ins_drum_kick.brr"

    .global data_snd_smp_ins_drum_snare
data_snd_smp_ins_drum_snare:
    .incbin "sound/ins/ins_drum_snare.brr"

    .global data_snd_smp_ins_drum_hihat
data_snd_smp_ins_drum_hihat:
    .incbin "sound/ins/ins_drum_hihat.brr"

    .global data_snd_smp_ins_drum_cymbals
data_snd_smp_ins_drum_cymbals:
    .incbin "sound/ins/ins_drum_cymbals.brr"

;    .global data_snd_smp_ins_drum_tom
;data_snd_smp_ins_drum_tom:
;    .incbin "sound/ins/ins_drum_tom.brr"

    .global data_snd_smp_ins_drum_clap
data_snd_smp_ins_drum_clap:
    .incbin "sound/ins/ins_drum_clap.brr"

    .global data_snd_smp_ins_drum_stick
data_snd_smp_ins_drum_stick:
    .incbin "sound/ins/ins_drum_stick.brr"