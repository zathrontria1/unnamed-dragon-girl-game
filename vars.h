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
extern ZP bool system_in_vblank;

extern ZP uint16_t system_current_routine;
extern ZP uint16_t system_target_routine;

extern ZP uint32_t system_frames_elapsed;

extern ZP uint16_t system_time_h;
extern ZP uint8_t system_time_m;
extern ZP uint8_t system_time_s;
extern ZP uint8_t system_time_subframe;

extern ZP uint32_t system_frames_lag;
extern bool system_dont_count_lag; // set to 1 to not increment the lag counter during e.g. creating a text box

extern ZP uint16_t system_nmis_counted;
extern ZP bool system_use_alternate_nmi;

extern ZP bool system_fblank_enabled;

extern ZP bool system_game_paused;

extern ZP void * system_loop_func_ptr;

extern ZP bool system_suppress_odd_transfers;

// Shadow buffers
extern ZP uint8_t shadow_nmitimen;

extern ZP uint8_t shadow_fblank_enable;
extern ZP int16_t shadow_brightness;
extern ZP int16_t shadow_brightness_change;

extern ZP uint8_t shadow_mosaic;

extern ZP uint8_t shadow_cgwsub;
extern ZP uint8_t shadow_cgadsub;
extern ZP uint8_t shadow_coldata_r;
extern ZP uint8_t shadow_coldata_g;
extern ZP uint8_t shadow_coldata_b;

extern ZP uint8_t shadow_stat77;

extern ZP uint8_t shadow_hdmaen;

extern NEAR union oam_buffer shadow_oam;
extern NEAR union cgram_full shadow_cgram; // 256 palette entries, 2 bytes wide each

// Event flags
extern uint8_t event_flags_global[EVENT_FLAG_GLOBAL_MAX];
extern uint8_t event_flags_local[EVENT_FLAG_LOCAL_MAX];

extern int16_t event_interaction_x;
extern int16_t event_interaction_y;

extern uint16_t event_in_combat;
extern uint16_t event_in_combat_shadow;

extern uint16_t event_tutorial_shown;
