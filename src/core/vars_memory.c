#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "sprites/spr_player_lut.h"
#include "sprites/boss/spr_boss_placeholder_lut.h"

#include "data_tables.h"
#include "data_tables_lut.h"
#include "data_cutscenes.h"
#include "data_metaspr_list.h"

#include "consts.h"
#include "consts_snd.h"
#include "sound/seq/seq_test.h"
#include "sound/seq/seq_module1.h"
#include "sound/seq/seq_aryx.h"

const uint32_t const_zero[] = 
{
    0, 0, 0, 0, 0, 0, 0, 0
};

// System general shadows and variables
ZP bool system_in_vblank;

uint16_t system_current_routine;
uint16_t system_target_routine;

uint32_t system_frames_elapsed;

uint16_t system_time_h;
uint8_t system_time_m;
uint8_t system_time_s;
uint8_t system_time_subframe;

uint32_t system_frames_lag;
bool system_dont_count_lag; // set to 1 to not increment the lag counter during e.g. creating a text box

ZP uint16_t system_nmis_counted;
ZP bool system_use_alternate_nmi;

ZP bool system_use_long_vblank;

ZP bool system_game_paused;

void * system_loop_func_ptr;

ZP bool system_suppress_odd_transfers;

// Shadow buffers
ZP uint8_t shadow_nmitimen;

// Dedicated zero-page scratch space for NMI routines (avoids compiler pseudoregisters)
ZP uint8_t nmi_snd_scratch_ptr[4];
ZP uint8_t nmi_snd_scratch_ptr2[4];
ZP uint16_t nmi_snd_scratch_len;
ZP uint16_t nmi_snd_scratch_temp;


// Additional variables to enable the ability to smoothly alter INIDISP. 
ZP uint8_t shadow_fblank_enable; // Valid values are 0x80 and 0x00. Will be OR'd with the highest 4 bits of the below.
ZP int16_t shadow_brightness; // 12-bit (4+8). Must be clamped to positive values only, but otherwise has to be an int
ZP int16_t shadow_brightness_change; // Must be signed to be able to describe a negative delta. -256/+256 is equivalent to 1 INIDISP brightness level

ZP uint8_t shadow_mosaic;

ZP uint8_t shadow_cgwsub;
ZP uint8_t shadow_cgadsub;
ZP uint8_t shadow_coldata_r;
ZP uint8_t shadow_coldata_g;
ZP uint8_t shadow_coldata_b;

ZP uint8_t shadow_stat77;

ZP uint8_t shadow_hdmaen;

union oam_buffer shadow_oam;
union cgram_full shadow_cgram; // 256 palette entries, 2 bytes wide each

// Event flags
uint8_t event_flags_global[EVENT_FLAG_GLOBAL_MAX];
uint8_t event_flags_local[EVENT_FLAG_LOCAL_MAX];

int16_t event_interaction_x;
int16_t event_interaction_y;

uint16_t event_in_combat;
uint16_t event_in_combat_shadow;

uint16_t event_tutorial_shown;
