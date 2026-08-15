#ifndef CONSTS_SND_H
#define CONSTS_SND_H

#define SND_SIG_CLEAR 0x00
#define SND_SIG_BUSY 0xff

#define SND_CMD_NOP 0x00

// Primary commands (0x01..0x14)
#define SND_CMD_DATA_SAMPLE_UPLOAD 0x01
#define SND_CMD_DATA_SAMPLE_SET_TUNE 0x02
#define SND_CMD_SEQ_UPLOAD 0x03

#define SND_CMD_SFX_PLAY 0x04
#define SND_CMD_SFX_PLAY_EXTEND 0x05
#define SND_CMD_SFX_STOP 0x06

#define SND_CMD_MUS_START 0x07
#define SND_CMD_MUS_PAUSE 0x08
#define SND_CMD_MUS_STOP 0x09
#define SND_CMD_MUS_SET_TEMPO 0x0a
#define SND_CMD_MUS_SET_SPEED 0x0b
#define SND_CMD_MUS_SET_OUTPUTMODE 0x0c

#define SND_CMD_SET_MUSIC_VOL 0x0d
#define SND_CMD_SET_SFX_VOL 0x0e
#define SND_CMD_SET_VOICE_VOL 0x0f

#define SND_CMD_STREAM_STOP 0x10
#define SND_CMD_STREAM_UPLOAD 0x11
#define SND_CMD_DSP_SET 0x12
#define SND_CMD_DIR_RESET 0x13
#define SND_CMD_SOFTRESET 0x14
#define SND_CMD_LOCK_SFX 0x15
#define SND_CMD_MUS_INS_RESET 0x16

// Multi-phase handshake sub-commands (0x20..0x26)
#define SND_CMD_DATA_SAMPLE_UPLOAD_TICK 0x20
#define SND_CMD_DATA_SAMPLE_UPLOAD_SLOT 0x21
#define SND_CMD_DATA_SAMPLE_UPLOAD_SAMPLERATE 0x22
#define SND_CMD_DATA_SAMPLE_UPLOAD_LOOPSTART 0x23
#define SND_CMD_DATA_SAMPLE_UPLOAD_ADSR 0x24

#define SND_CMD_SEQ_UPLOAD_TRACK 0x25

#define SND_CMD_SFX_PLAY_EXTEND_VOLDATA 0x26

// Instruments: 0x00-0x0f (0-15)
#define INS_TONE_SQUARE 0x00
#define INS_BASS 0x01
#define INS_PIANO 0x02

#define INS_GUITAR_ACOS 0x03
#define INS_GUITAR_DIST 0x04

#define INS_FLUTE 0x05
#define INS_TRUMPET 0x06
#define INS_SAX 0x07

#define INS_STRINGS 0x08
#define INS_CELLO 0x09
#define INS_VIOLIN 0x0a

#define INS_MARIMBA 0x0b

// Drums/One-shot: 0x10-0x1f (16-31)
#define INS_DRUM_KICK 0x10
#define INS_DRUM_SNARE 0x11
#define INS_DRUM_HIHAT 0x12
#define INS_DRUM_CYMBALS 0x13
#define INS_DRUM_TOM 0x14
#define INS_DRUM_CLAP 0x15
#define INS_DRUM_STICK 0x16

// Sound effects: 0x20-0x3f (32-62)
#define SFX_UI_CONFIRM 0x20
#define SFX_ATK_PUNCH 0x21
#define SFX_ATK_SWING 0x22
#define SFX_ATK_SPLASH 0x23
#define SFX_ATK_SPLAT_HIT 0x24
#define SFX_ATK_FIRE_BREATH 0x25
#define SFX_ATK_FIRE_CRACKLE 0x26
#define SFX_MOV_FOOTSTEP 0x27
#define SFX_INTERACT_SWITCH 0x28
#define SFX_DROP_COIN 0x29
#define SFX_DROP_BOUNCE 0x2a

// The effect denoting a stream is 63
#define STR_STREAM_DATA 0x3f

/*
    Below 0x80 = 1-byte note key-on (uses track sticky instrument, volume, ADSR, duration).
    0x80 - 0x8F = 1-byte short wait (0 to 15 ticks).
    0x90 - 0x9b, 0xFF = extended control and parameterized opcodes.
*/

// Short wait opcodes (1-byte opcodes for 0..15 ticks)
#define SEQ_WAIT_0   0x80
#define SEQ_WAIT_1   0x81
#define SEQ_WAIT_2   0x82
#define SEQ_WAIT_3   0x83
#define SEQ_WAIT_4   0x84
#define SEQ_WAIT_5   0x85
#define SEQ_WAIT_6   0x86
#define SEQ_WAIT_7   0x87
#define SEQ_WAIT_8   0x88
#define SEQ_WAIT_9   0x89
#define SEQ_WAIT_10  0x8a
#define SEQ_WAIT_11  0x8b
#define SEQ_WAIT_12  0x8c
#define SEQ_WAIT_13  0x8d
#define SEQ_WAIT_14  0x8e
#define SEQ_WAIT_15  0x8f

// Parameterized & control macros
#define SEQ_WAIT(ticks)          0x90, (uint8_t)(ticks)
#define SEQ_SET_INS(ins)         0x91, (uint8_t)(ins)
#define SEQ_SET_VOL(l, r)        0x92, (uint8_t)(l), (uint8_t)(r)
#define SEQ_SET_ADSR(l, h)       0x93, (uint8_t)(l), (uint8_t)(h)
#define SEQ_SET_DURATION(ticks)  0x94, (uint8_t)(ticks)
#define SEQ_PLAY_DRUM(drum_id)   0x95, (uint8_t)(drum_id)
#define SEQ_SET_LOOP(count)      0x96, (uint8_t)(count)
#define SEQ_LOOP                 0x97
#define SEQ_SET_RESTART          0x98
#define SEQ_SET_SPEED(speed)     0x99, (uint8_t)(speed)
#define SEQ_SET_TEMPO(bpm)       0x9a, (uint8_t)(bpm)
#define SEQ_NOTE_CUT             0x9b
#define SEQ_RESTART              0xff

// Raw opcode numbers for parser
#define SEQ_OPCODE_WAIT_BASE     0x80
#define SEQ_OPCODE_WAIT_EXT      0x90
#define SEQ_OPCODE_SET_INS       0x91
#define SEQ_OPCODE_SET_VOL       0x92
#define SEQ_OPCODE_SET_ADSR      0x93
#define SEQ_OPCODE_SET_DURATION  0x94
#define SEQ_OPCODE_PLAY_DRUM     0x95
#define SEQ_OPCODE_SET_LOOP      0x96
#define SEQ_OPCODE_LOOP          0x97
#define SEQ_OPCODE_SET_RESTART   0x98
#define SEQ_OPCODE_SET_SPEED     0x99
#define SEQ_OPCODE_SET_TEMPO     0x9a
#define SEQ_OPCODE_NOTE_CUT      0x9b
#define SEQ_OPCODE_RESTART       0xff

// Piano key freq names to MIDI note
#define NOTE_C0 12+(0*12)
#define NOTE_Cs0 13+(0*12)
#define NOTE_D0 14+(0*12)
#define NOTE_Ds0 15+(0*12)
#define NOTE_E0 16+(0*12)
#define NOTE_F0 17+(0*12)
#define NOTE_Fs0 18+(0*12)
#define NOTE_G0 19+(0*12)
#define NOTE_Gs0 20+(0*12)
#define NOTE_A0 21+(0*12)
#define NOTE_As0 22+(0*12)
#define NOTE_B0 23+(0*12)

#define NOTE_C1 12+(1*12)
#define NOTE_Cs1 13+(1*12)
#define NOTE_D1 14+(1*12)
#define NOTE_Ds1 15+(1*12)
#define NOTE_E1 16+(1*12)
#define NOTE_F1 17+(1*12)
#define NOTE_Fs1 18+(1*12)
#define NOTE_G1 19+(1*12)
#define NOTE_Gs1 20+(1*12)
#define NOTE_A1 21+(1*12)
#define NOTE_As1 22+(1*12)
#define NOTE_B1 23+(1*12)

#define NOTE_C2 12+(2*12)
#define NOTE_Cs2 13+(2*12)
#define NOTE_D2 14+(2*12)
#define NOTE_Ds2 15+(2*12)
#define NOTE_E2 16+(2*12)
#define NOTE_F2 17+(2*12)
#define NOTE_Fs2 18+(2*12)
#define NOTE_G2 19+(2*12)
#define NOTE_Gs2 20+(2*12)
#define NOTE_A2 21+(2*12)
#define NOTE_As2 22+(2*12)
#define NOTE_B2 23+(2*12)

#define NOTE_C3 12+(3*12)
#define NOTE_Cs3 13+(3*12)
#define NOTE_D3 14+(3*12)
#define NOTE_Ds3 15+(3*12)
#define NOTE_E3 16+(3*12)
#define NOTE_F3 17+(3*12)
#define NOTE_Fs3 18+(3*12)
#define NOTE_G3 19+(3*12)
#define NOTE_Gs3 20+(3*12)
#define NOTE_A3 21+(3*12)
#define NOTE_As3 22+(3*12)
#define NOTE_B3 23+(3*12)

#define NOTE_C4 12+(4*12)
#define NOTE_Cs4 13+(4*12)
#define NOTE_D4 14+(4*12)
#define NOTE_Ds4 15+(4*12)
#define NOTE_E4 16+(4*12)
#define NOTE_F4 17+(4*12)
#define NOTE_Fs4 18+(4*12)
#define NOTE_G4 19+(4*12)
#define NOTE_Gs4 20+(4*12)
#define NOTE_A4 21+(4*12)
#define NOTE_As4 22+(4*12)
#define NOTE_B4 23+(4*12)

#define NOTE_C5 12+(5*12)
#define NOTE_Cs5 13+(5*12)
#define NOTE_D5 14+(5*12)
#define NOTE_Ds5 15+(5*12)
#define NOTE_E5 16+(5*12)
#define NOTE_F5 17+(5*12)
#define NOTE_Fs5 18+(5*12)
#define NOTE_G5 19+(5*12)
#define NOTE_Gs5 20+(5*12)
#define NOTE_A5 21+(5*12)
#define NOTE_As5 22+(5*12)
#define NOTE_B5 23+(5*12)

#define NOTE_C6 12+(6*12)
#define NOTE_Cs6 13+(6*12)
#define NOTE_D6 14+(6*12)
#define NOTE_Ds6 15+(6*12)
#define NOTE_E6 16+(6*12)
#define NOTE_F6 17+(6*12)
#define NOTE_Fs6 18+(6*12)
#define NOTE_G6 19+(6*12)
#define NOTE_Gs6 20+(6*12)
#define NOTE_A6 21+(6*12)
#define NOTE_As6 22+(6*12)
#define NOTE_B6 23+(6*12)

#define NOTE_C7 12+(7*12)
#define NOTE_Cs7 13+(7*12)
#define NOTE_D7 14+(7*12)
#define NOTE_Ds7 15+(7*12)
#define NOTE_E7 16+(7*12)
#define NOTE_F7 17+(7*12)
#define NOTE_Fs7 18+(7*12)
#define NOTE_G7 19+(7*12)
#define NOTE_Gs7 20+(7*12)
#define NOTE_A7 21+(7*12)
#define NOTE_As7 22+(7*12)
#define NOTE_B7 23+(7*12)

#define NOTE_C8 12+(8*12)
#define NOTE_Cs8 13+(8*12)
#define NOTE_D8 14+(8*12)
#define NOTE_Ds8 15+(8*12)
#define NOTE_E8 16+(8*12)
#define NOTE_F8 17+(8*12)
#define NOTE_Fs8 18+(8*12)
#define NOTE_G8 19+(8*12)
#define NOTE_Gs8 20+(8*12)
#define NOTE_A8 21+(8*12)
#define NOTE_As8 22+(8*12)
#define NOTE_B8 23+(8*12)

// Check data_tables.h - this must be in array order
#define STREAM_SILENCE 0

#define STREAM_TYPEWRITER 1
#define STREAM_HISS 2

#define STREAM_VOICE_HURT_1 3
#define STREAM_VOICE_HURT_2 4

#define STREAM_VOICE_ATTACK_1 5
#define STREAM_VOICE_ATTACK_2 6

//#define STREAM_VOICE_UPGRADE_SUCCESS_1 5
//#define STREAM_VOICE_UPGRADE_SUCCESS_2 6

//#define STREAM_VOICE_TREASURE_1 7
//#define STREAM_VOICE_TREASURE_2 8

#endif // CONSTS_SND_H

