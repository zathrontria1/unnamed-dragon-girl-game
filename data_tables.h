#include "consts_snd.h"

#include "data_strings.h"

const struct level_palette_list data_palette_list_0 = 
{
    {
        (void *)&data_palette_ui,
        (void *)&data_palette_dungeon_0,
        (void *)&data_palette_dungeon_1,
        (void *)&data_palette_dungeon_2,
        (void *)&data_palette_blank,
        (void *)&data_palette_blank,
        (void *)&data_palette_blank,
        (void *)&data_palette_blank,

        (void *)&data_palette_player,
        (void *)&data_palette_common_0,
        (void *)&data_palette_common_1,
        (void *)&data_palette_blank,
        (void *)&data_palette_blank,
        (void *)&data_palette_blank,
        (void *)&data_palette_blank,
        (void *)&data_palette_dungeon_0
    }
};

// Enemy data 
const struct enemy_data data_enemy_stats_slime = 
{
    15, 1, 1, 100, 200, 16, 16
};

const struct enemy_data data_enemy_stats_lizardman = 
{
    210, 2, 2, 200, 300, 16, 16
};
const struct enemy_data data_enemy_stats_lizardman_archer = 
{
    140, 2, 2, 200, 300, 16, 16
};
const struct enemy_data data_enemy_stats_lizardman_lilsis = 
{
    // Placeholder; this is supposed to be the alternate player
    350, 1, 1, 0, 0, 16, 16
};

// Stream defs
const struct sound_stream_data data_stream_table[] = 
{
    {(void *)&data_snd_stream_silence, 288, false, 0}, // STREAM_SILENCE

    {(void *)&data_snd_stream_voice_hurt_1, 1440, false, 0}, // STREAM_VOICE_HURT_1
    {(void *)&data_snd_stream_voice_hurt_2, 864, false, 0}, // STREAM_VOICE_HURT_2

    {(void *)&data_snd_stream_voice_attack_1, 864, false, 0}, // STREAM_VOICE_ATTACK_1
    {(void *)&data_snd_stream_voice_attack_2, 864, false, 0}, // STREAM_VOICE_ATTACK_2

    {(void *)&data_snd_stream_voice_upgrade_success_1, 2592, false, 0}, // STREAM_VOICE_UPGRADE_SUCCESS_1
    {(void *)&data_snd_stream_voice_upgrade_success_2, 3456, false, 0}, // STREAM_VOICE_UPGRADE_SUCCESS_2

    {(void *)&data_snd_stream_voice_treasure_1, 2880, false, 0}, // STREAM_VOICE_TREASURE_1
    {(void *)&data_snd_stream_voice_treasure_2, 2592, false, 0}, // STREAM_VOICE_TREASURE_2

    {(void *)&data_snd_stream_typewriter, 576, true, 0}, // STREAM_TYPEWRITER
    {(void *)&data_snd_stream_hiss, 2304, false, 0}, // STREAM_HISS
};

// wav2brr produces files with 2 byte headers. Take into account this and subtract 2 from file length on disk.
// pitch value = (actual rate * 4096) / 32000
// tick values will be automatically generated at runtime if set to 0
// a tick value of 255 means infinite/until forced key off

// an entry with a length 0 terminates the list

// ADSR L	$X5	EDDD AAAA	ADSR enable (E), decay rate (D), attack rate (A).
// ADSR H	$X6	LLLR RRRR	Sustain level (SL), sustain rate (SR).

// Example: (0x0f | (0x03 << 4) | (0x01 << 7) | (0x17 << 8) | (0x07 << 13))

// B+21 cents =  1 + 0.0005946c = 1.0124866 
// rate / 1.0124866 = actual 
// 32000 / 1.0124866 = 31605.357 (treat as 31605)
// 16000 / 1.0124866 = 15802.678 (treat as 15803)
// 8000 / 1.0124866 = 7901.339 (treat as 7901)


const struct sample_list_entry_ins data_snd_instruments[] = 
{
    //{INS_TONE_SQUARE_B3, (void *)&data_snd_smp_ins_tone_square_b3, 81, ((31605l * 4096l) / 32000l), 0xf3ff, 30, 59},
    {INS_TONE_SQUARE, (void *)&data_snd_smp_ins_tone_square, 45, ((31605l * 4096l) / 32000l), 0x0000, 15, NOTE_B4},

    {INS_BASS, (void *)&data_snd_smp_ins_bass, 828, 0x0751, (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_C2},

    {INS_PIANO, (void *)&data_snd_smp_ins_piano, 2394, 0x08c7, (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_G3},

    {INS_GUITAR_ACOS, (void *)&data_snd_smp_ins_guitar_acos, 882, 0x12e6, (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_G3},

    {INS_GUITAR_DIST, (void *)&data_snd_smp_ins_guitar_dist, 2340, 0x08ca, (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_C3},

    {INS_FLUTE, (void *)&data_snd_smp_ins_flute, 711, 0x0465, (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_C4},
    {INS_TRUMPET, (void *)&data_snd_smp_ins_trumpet, 495, ((31605l * 4096l) / 32000l), (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_B4},
    {INS_SAX, (void *)&data_snd_smp_ins_sax, 846, 0x0BDC, (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_Fs4},

    {INS_STRINGS, (void *)&data_snd_smp_ins_strings, 2691, 0x0803, (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_C5},
    {INS_CELLO, (void *)&data_snd_smp_ins_cello, 3474, 0x083F, (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_C3},
    {INS_VIOLIN, (void *)&data_snd_smp_ins_violin, 5112, 0x0844, (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_C5},

    {INS_MARIMBA, (void *)&data_snd_smp_ins_marimba, 900, 0x0E1D, (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 30, NOTE_C3},

    // D = 6, R = 16
    {INS_DRUM_KICK, (void *)&data_snd_smp_ins_drum_kick, 3600, ((32000l * 4096l) / 32000l), 0x0000, 0, 0},
    //{INS_DRUM_KICK, (void *)&data_snd_smp_ins_drum_kick, 2601, ((32000l * 4096l) / 32000l), 0xf6ef, 20, 60},

    // D = 6, R = 16
    {INS_DRUM_SNARE, (void *)&data_snd_smp_ins_drum_snare, 4050, ((32000l * 4096l) / 32000l), 0x0000, 0, 0},
    //{INS_DRUM_SNARE, (void *)&data_snd_smp_ins_drum_snare, 3600, ((32000l * 4096l) / 32000l), 0xf6ef, 22, 60},

    // D = 7, R = 1b
    {INS_DRUM_HIHAT, (void *)&data_snd_smp_ins_drum_hihat, 1125, ((32000l * 4096l) / 32000l), 0x0000, 0, 0},
    //{INS_DRUM_HIHAT, (void *)&data_snd_smp_ins_drum_hihat, 900, ((32000l * 4096l) / 32000l), 0xfbff, 6, 60},

    // D = 5, R = 19
    {INS_DRUM_CYMBALS, (void *)&data_snd_smp_ins_drum_cymbals, 2025, ((32000l * 4096l) / 32000l), 0x0000, 0, 0},
    //{INS_DRUM_CYMBALS, (void *)&data_snd_smp_ins_drum_cymbals, 2250, ((32000l * 4096l) / 32000l), 0xf5bf, 34, 60},

    // D = 4, R = 0e
    //{INS_DRUM_TOM, (void *)&data_snd_smp_ins_drum_tom, 2610, ((32000l * 4096l) / 32000l), 0xeecf, 108, 60},

    {INS_DRUM_CLAP, (void *)&data_snd_smp_ins_drum_clap, 1350, ((32000l * 4096l) / 32000l), 0x0000, 0, 60},

    {INS_DRUM_STICK, (void *)&data_snd_smp_ins_drum_stick, 450, ((16000l * 4096l) / 32000l), 0x0000, 0, 60},
    
    {0, 0, 0, 0x1000, 0x0000, 0, 0},
};

const struct sample_list_entry data_snd_samples[] = 
{
    {SFX_UI_CONFIRM, (void *)&data_snd_smp_sfx_msgclick, 225, ((8000l * 4096l) / 32000l), 0x0000, 0},
    {SFX_ATK_PUNCH, (void *)&data_snd_smp_sfx_punch, 1935, ((8000l * 4096l) / 32000l), 0x0000, 0},
    {SFX_MOV_FOOTSTEP, (void *)&data_snd_smp_sfx_footstep, 549, ((8000l * 4096l) / 32000l), 0x0000, 0},
    {SFX_ATK_SWING, (void *)&data_snd_smp_sfx_whoosh, 1224, ((8000l * 4096l) / 32000l), 0x0000, 0},
    {SFX_DROP_COIN, (void *)&data_snd_smp_sfx_coin, 369, ((32000l * 4096l) / 32000l), 0xfaff, 30},
    {SFX_INTERACT_SWITCH, (void *)&data_snd_smp_sfx_switch, 414, ((8000l * 4096l) / 32000l), 0x0000, 0},
    {SFX_ATK_SPLASH, (void *)&data_snd_smp_sfx_splash, 1116, ((8000l * 4096l) / 32000l), 0x0000, 0},
    {SFX_ATK_SPLAT_HIT, (void *)&data_snd_smp_sfx_splathit, 1116, ((8000l * 4096l) / 32000l), 0x0000, 0},
    {SFX_ATK_FIRE_BREATH, (void *)&data_snd_smp_sfx_flamestream, 2250, ((8000l * 4096l) / 32000l), 0x0000, 255},
    {SFX_ATK_FIRE_CRACKLE, (void *)&data_snd_smp_sfx_firecrackle, 576, ((8000l * 4096l) / 32000l), 0xfaff, 30},
    {SFX_DROP_BOUNCE, (void *)&data_snd_smp_sfx_bounce, 153, ((32000l * 4096l) / 32000l), 0xfaff, 15},
    {0, 0, 0, 0x1000, 0x0000, 0},
};

// Interactable lists should look like this:
// ID, high X, high Y, flag OR string pointer
// Terminate with NULL id
const struct obj_list_entry_interactable data_interactables_debug0[] = 
{
    {OBJID_INTERACTABLE_SWITCH_WALL, 440, 432-8, 0},
    {OBJID_INTERACTABLE_BLOCKER_DOOR_NS, 496, 432, 0}, 
    //{OBJID_INTERACTABLE_BLOCKER_DOOR_NS, 512, 432, 0}, 
    //{OBJID_INTERACTABLE_BLOCKER_DOOR_EW, 384, 496, 0}, 
    //{OBJID_INTERACTABLE_BLOCKER_DOOR_EW, 384, 512, 0}, 
    {OBJID_INTERACTABLE_BLOCKER_DOOR_EW, 384, 512, 0}, 
    //{OBJID_INTERACTABLE_BLOCKER_DOOR_EW, 624, 496, 0}, 
    //{OBJID_INTERACTABLE_BLOCKER_DOOR_EW, 624, 512, 0}, 
    {OBJID_INTERACTABLE_BLOCKER_DOOR_EW, 624, 512, 0}, 
    {OBJID_INTERACTABLE_BLOCKER_DOOR_NS, 496, 640, 0}, 
    //{OBJID_INTERACTABLE_BLOCKER_DOOR_NS, 512, 624, 0}, 
    {OBJID_INTERACTABLE_SIGN_WALL, 592, 432-8, (void *)&STR_MSG_TEST_MULTILINE}, 
    {OBJID_INTERACTABLE_SIGN_WALL, 480, 432-8, (void *)&STR_MSG_TEST_MULTIPAGE}, 

    {OBJID_INTERACTABLE_LEVEL_WARP, 496, 32, (void *)&data_level_test_1}, 

    {OBJID_INTERACTABLE_TREASURECHEST, 312, 320, (void *)5000l}, 
    {OBJID_INTERACTABLE_TREASURECHEST, 312, 320-32, (void *)100000l}, 
    {OBJID_INTERACTABLE_TREASURECHEST, 312-32, 320, (void *)500000l}, 
    {OBJID_INTERACTABLE_TREASURECHEST, 312+32, 320, (void *)10000000l}, 
    {OBJID_INTERACTABLE_TREASURECHEST, 312, 320+32, (void *)50000000l}, 
    {OBJID_NULL, 0, 0, 0}, 
};

const struct obj_list_entry_interactable data_interactables_debug1[] = 
{
    {OBJID_NULL, 0, 0, 0}, 
};


// Spawn lists should look like this:
// ID, high X, high Y (offsets from spawn list)
// Terminate with NULL id
const struct obj_list_entry_spawns data_spawnlist_debug0[] = {
    {OBJID_SLIME, 20+0, 48, 16},
    {OBJID_SLIME, 20+56, 48, 16},
    {OBJID_SLIME, 20+112, 48, 16},
    {OBJID_SLIME, 20+168, 48, 16},
    {OBJID_SLIME, 20+0, 64, 16},
    {OBJID_SLIME, 20+56, 64, 16},
    {OBJID_SLIME, 20+112, 64, 16},
    {OBJID_SLIME, 20+168, 64, 16},
    {OBJID_NULL, 0, 0, 0},
};

const struct obj_list_entry_spawns data_spawnlist_debug1[] = {
    {OBJID_LIZARDMAN, 20+0, 48, 16},
    {OBJID_LIZARDMAN, 20+56, 48, 16},
    {OBJID_LIZARDMAN, 20+112, 48, 16},
    {OBJID_LIZARDMAN, 20+168, 48, 16},
    {OBJID_LIZARDMAN, 20+0, 64, 16},
    {OBJID_LIZARDMAN, 20+56, 64, 16},
    {OBJID_LIZARDMAN, 20+112, 64, 16},
    {OBJID_LIZARDMAN, 20+168, 64, 16},
    {OBJID_NULL, 0, 0, 0},
};

const struct obj_list_entry_spawners data_spawners_debug0[] = {
    {OBJID_SPAWNER_ENEMY, 400+15, 448+15, 224-31, 176-31, 384, 400, 256, 224, (void *)&data_spawnlist_debug0},
    {OBJID_SPAWNER_ENEMY, 16+15, 448+15, 224-31, 176-31, 0, 400, 256, 224, (void *)&data_spawnlist_debug1},
    {OBJID_SPAWNER_ENEMY, 400+15, 48+15, 224-31, 176-31, 384, 0, 256, 224, (void *)&data_spawnlist_debug0},
    {OBJID_SPAWNER_ENEMY, 400+15, 848+15, 224-31, 176-31, 384, 800, 256, 224, (void *)&data_spawnlist_debug1},
    // TODO: Water collision type
    //{OBJID_SPAWNER_ENEMY, 784, 448, 224, 176, 768, 400, 256, 224, (void *)&data_spawnlist_debug0},
    {OBJID_NULL, 0, 0, 1, 1, 0, 0, 0, 0, 0},
};

const struct obj_list_entry_spawners data_spawners_debug1[] = {
    {OBJID_NULL, 0, 0, 1, 1, 0, 0, 0, 0, 0},
};

// Level data structs
const struct level_data data_level_test_0 = {
    504,
    504,

    (void *)&data_bg_dungeon_lz4,
    (void *)&data_palette_list_0,

    (void *)&data_map_debug0,
    (void *)&data_map_lut_dungeon,
    (void *)&data_map_lut_dungeon_col,

    (void *)&data_spawners_debug0,
    (void *)&data_interactables_debug0,

    (void *)&data_bg_map_dungeon_0_8bpp_lz4,
    (void *)&data_palette_map_0_8bpp,

    (void *)&STR_LEVELNAME_DEBUG_B1F
};

const struct level_data data_level_test_1 = {
    56,
    864,

    (void *)&data_bg_dungeon_lz4,
    (void *)&data_palette_list_0,

    (void *)&data_map_debug1,
    (void *)&data_map_lut_dungeon,
    (void *)&data_map_lut_dungeon_col,
    
    (void *)&data_spawners_debug1,
    (void *)&data_interactables_debug1,

    (void *)&data_bg_map_dungeon_1_8bpp_lz4,
    (void *)&data_palette_map_1_8bpp,

    (void *)&STR_LEVELNAME_DEBUG_B2F
};

const struct level_data data_level_test_2 = {
    504,
    504,
    
    (void *)&data_bg_dungeon_lz4,
    (void *)&data_palette_list_0,

    (void *)&data_map_debug0,
    (void *)&data_map_lut_dungeon,
    (void *)&data_map_lut_dungeon_col,

    (void *)&data_spawners_debug0,
    (void *)&data_interactables_debug0,

    (void *)&data_bg_map_dungeon_0_8bpp_lz4,
    (void *)&data_palette_map_0_8bpp,

    (void *)&STR_LEVELNAME_DEBUG_B3F
};
