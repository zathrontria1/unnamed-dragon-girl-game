#include <stdint.h>
#include <stdbool.h> // DO NOT REMOVE THIS

#include "consts.h"
#include "defs_objects.h"
#include "defs_structs.h"

#include "vars_extern.h"

// For DMA/HDMA copies
extern const uint32_t const_zero[8];
extern const uint8_t const_sprite_offscreen;

// System general shadows and variables
ZP extern bool system_in_vblank;

ZP extern uint16_t system_current_routine;
ZP extern uint16_t system_target_routine;

ZP extern uint32_t system_frames_elapsed;

ZP extern uint16_t system_time_h;
ZP extern uint8_t system_time_m;
ZP extern uint8_t system_time_s;
ZP extern uint8_t system_time_subframe;

ZP extern uint32_t system_frames_lag;
extern bool system_dont_count_lag; // set to 1 to not increment the lag counter during e.g. creating a text box

ZP extern uint16_t system_nmis_counted;
ZP extern bool system_use_alternate_nmi;

ZP extern bool system_fblank_enabled;

ZP extern bool system_game_paused;

ZP extern void * system_loop_func_ptr;

ZP extern bool system_suppress_odd_transfers;

// Shadow buffers
ZP extern uint8_t shadow_nmitimen;
ZP extern uint8_t shadow_inidisp;
ZP extern int8_t shadow_inidisp_change;

ZP extern uint8_t shadow_mosaic;

ZP extern uint8_t shadow_cgwsub;
ZP extern uint8_t shadow_cgadsub;
ZP extern uint8_t shadow_coldata_r;
ZP extern uint8_t shadow_coldata_g;
ZP extern uint8_t shadow_coldata_b;

ZP extern uint8_t shadow_stat77;

ZP extern uint8_t shadow_hdmaen;

NEAR extern union oam_buffer shadow_oam;
NEAR extern union cgram_full shadow_cgram; // 256 palette entries, 2 bytes wide each

// Event flags
extern uint8_t event_flags_global[EVENT_FLAG_GLOBAL_MAX];
extern uint8_t event_flags_local[EVENT_FLAG_LOCAL_MAX];

extern int16_t event_interaction_x;
extern int16_t event_interaction_y;

extern uint16_t event_in_combat;
extern uint16_t event_in_combat_shadow;

extern uint16_t event_tutorial_shown;
