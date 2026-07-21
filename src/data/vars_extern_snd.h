#ifndef VARS_EXTERN_SND_H
#define VARS_EXTERN_SND_H

#include <stdint.h>
#include "consts.h"
#include "defs_structs.h"

extern HUGE const uint8_t data_soundengine_binary[];

// Sequence data
extern HUGE const struct seq_command data_seq_test_t1[];
extern HUGE const struct seq_command data_seq_test_t2[];
extern HUGE const struct seq_command data_seq_test_t3[];
extern HUGE const struct seq_command data_seq_test_t4[];
extern HUGE const struct seq_command data_seq_test_t5[];
extern HUGE const struct seq_command data_seq_test_t6[];

extern HUGE const struct sample_list_entry_ins data_snd_instruments[];
extern HUGE const struct sample_list_entry data_snd_samples[];

// SFX
extern HUGE const uint8_t data_snd_smp_sfx_whoosh[];
extern HUGE const uint8_t data_snd_smp_sfx_punch[];
extern HUGE const uint8_t data_snd_smp_sfx_footstep[];
extern HUGE const uint8_t data_snd_smp_sfx_coin[];
extern HUGE const uint8_t data_snd_smp_sfx_msgclick[];
extern HUGE const uint8_t data_snd_smp_sfx_switch[];
extern HUGE const uint8_t data_snd_smp_sfx_splash[];
extern HUGE const uint8_t data_snd_smp_sfx_splathit[];
extern HUGE const uint8_t data_snd_smp_sfx_flamestream[];
extern HUGE const uint8_t data_snd_smp_sfx_firecrackle[];
extern HUGE const uint8_t data_snd_smp_sfx_bounce[];

// Instruments
extern HUGE const uint8_t data_snd_smp_ins_tone_square[];

extern HUGE const uint8_t data_snd_smp_ins_bass[];

extern HUGE const uint8_t data_snd_smp_ins_piano[];

extern HUGE const uint8_t data_snd_smp_ins_guitar_acos[];
extern HUGE const uint8_t data_snd_smp_ins_guitar_dist[];

extern HUGE const uint8_t data_snd_smp_ins_flute[];
extern HUGE const uint8_t data_snd_smp_ins_trumpet[];
extern HUGE const uint8_t data_snd_smp_ins_sax[];

extern HUGE const uint8_t data_snd_smp_ins_strings[];
extern HUGE const uint8_t data_snd_smp_ins_cello[];
extern HUGE const uint8_t data_snd_smp_ins_violin[];

extern HUGE const uint8_t data_snd_smp_ins_marimba[];

// Drums/one-shot instruments
extern HUGE const uint8_t data_snd_smp_ins_drum_kick[];
extern HUGE const uint8_t data_snd_smp_ins_drum_snare[];
extern HUGE const uint8_t data_snd_smp_ins_drum_hihat[];
extern HUGE const uint8_t data_snd_smp_ins_drum_cymbals[];
extern HUGE const uint8_t data_snd_smp_ins_drum_clap[];
extern HUGE const uint8_t data_snd_smp_ins_drum_stick[];

// Streamed audio
// Player voice
extern HUGE const uint8_t data_snd_stream_voice_hurt_1[];
extern HUGE const uint8_t data_snd_stream_voice_hurt_2[];

extern HUGE const uint8_t data_snd_stream_voice_attack_1[];
extern HUGE const uint8_t data_snd_stream_voice_attack_2[];

/*extern HUGE const uint8_t data_snd_stream_voice_upgrade_success_1[];
extern HUGE const uint8_t data_snd_stream_voice_upgrade_success_2[];

extern HUGE const uint8_t data_snd_stream_voice_treasure_1[];
extern HUGE const uint8_t data_snd_stream_voice_treasure_2[];*/


// Other streams
extern HUGE const uint8_t data_snd_stream_silence[];
extern HUGE const uint8_t data_snd_stream_typewriter[];
extern HUGE const uint8_t data_snd_stream_hiss[];

// Streamed audio table to make it easier to refer to them
extern HUGE const struct sound_stream_data data_stream_table[];

#endif // VARS_EXTERN_SND_H

