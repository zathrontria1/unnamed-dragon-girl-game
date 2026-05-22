#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "maps/map_lut_dungeon.h"
#include "maps/map_lut_dungeon_col.h"
#include "maps/map_debug0.h"
#include "maps/map_debug1.h"
#include "data_tables.h"
#include "data_tables_lut.h"
#include "data_metaspr_list.h"

#include "consts.h"
#include "consts_snd.h"
#include "sound/seq/seq_test.h"

// For SRAM management
const uint8_t const_sram_verify_str[] = "EIEIMUN!"; // Can use any 8 character string that isn't all 0x00 or 0xff. Will occupy 9 bytes in ROM
uint8_t sram_available_slots;

// For DMA/HDMA copies
const uint32_t const_zero = 0;
const uint8_t const_sprite_offscreen = 0xf0;

const uint8_t const_hdma_tm_msgbox[] = 
{
    UI_MSGBOX_ML_START * 4, TM_MODE1, 
    UI_MSGBOX_ML_START * 4, TM_MODE1, 
    UI_MSGBOX_HEIGHT * 8, TM_MODE1_MSGBOX, 
    1, TM_MODE1, 
    0, 
};

const uint16_t const_ui_textadvance_tilemapentries[] =
{
    0x0060 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
    0x0061 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
    0x0070 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
    0x0071 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
};

// System general shadows and variables
ZP uint16_t system_in_vblank;

ZP uint16_t system_current_routine;
ZP uint16_t system_target_routine;

ZP uint32_t system_frames_elapsed;
ZP uint32_t system_frames_lag;
uint16_t system_dont_count_lag; // set to 1 to not increment the lag counter during e.g. creating a text box

ZP uint8_t shadow_stat77;

ZP uint8_t shadow_inidisp;
ZP int8_t shadow_inidisp_change;

ZP uint8_t shadow_mosaic;

ZP uint8_t shadow_cgwsub;
ZP uint8_t shadow_cgadsub;
ZP uint8_t shadow_coldata_r;
ZP uint8_t shadow_coldata_g;
ZP uint8_t shadow_coldata_b;

ZP uint16_t system_nmis_counted;
ZP uint16_t system_use_alternate_nmi;

ZP uint16_t system_game_paused;

ZP void * system_loop_func_ptr;

ZP struct game_object * obj_player_pointer;

// Object system
ZP uint16_t obj_first_available;
struct game_object obj_general[OBJ_GENERAL_MAX_COUNT];

uint16_t obj_delete_queue[OBJ_GENERAL_MAX_COUNT];
uint16_t obj_delete_queue_count;

int16_t obj_player_index;
uint16_t obj_active_count;

uint16_t obj_next_uid;

// Enemy data that shouldn't be in the object area
uint16_t obj_enemies_defeated;
uint16_t obj_enemies_target_count;
uint16_t obj_enemies_max_count;

// Player only data that shouldn't be in the object area
uint16_t obj_player_attack_interval;
uint16_t obj_player_prev_facing;
uint8_t * obj_player_prev_sprframe;
uint16_t obj_player_active_fireballs;

uint16_t obj_player_health_regen_delay;
uint16_t obj_player_health_regen_interval;
uint16_t obj_player_health_regen_value;
uint16_t obj_player_health_regen_limit;

uint16_t obj_player_recovery_drop_pity;

// Hitbox data
ZP uint16_t obj_hitbox_player_first_available;
struct game_object obj_hitbox_player[OBJ_PLAYERHITBOX_MAX_COUNT];
uint16_t obj_hitbox_player_delete_queue[OBJ_PLAYERHITBOX_MAX_COUNT];
uint16_t obj_hitbox_player_delete_queue_count;
uint16_t obj_hitbox_count_player;

ZP uint16_t obj_hitbox_enemy_first_available;
struct game_object obj_hitbox_enemy[OBJ_ENEMYHITBOX_MAX_COUNT];
uint16_t obj_hitbox_enemy_delete_queue[OBJ_ENEMYHITBOX_MAX_COUNT];
uint16_t obj_hitbox_enemy_delete_queue_count;
uint16_t obj_hitbox_count_enemy;

// Event flags
uint8_t event_flags_global[EVENT_FLAG_GLOBAL_MAX];
uint8_t event_flags_local[EVENT_FLAG_LOCAL_MAX];

int16_t event_interaction_x;
int16_t event_interaction_y;

uint16_t event_in_combat;
uint16_t event_in_combat_shadow;

uint16_t event_tutorial_shown;

// Random numbers
ZP int8_t rand_array[3];
uint16_t rand_seeded;

// DMA system
NEAR struct dma_entry dma_queue[DMA_QUEUE_MAX_ENTRIES];
uint16_t dma_queue_count;
uint16_t dma_queue_length;

// Input system
uint16_t input_pad0;
uint16_t input_pad0_new;
//uint16_t input_pad1;
//uint16_t input_pad1_new;

// Map system
const struct level_data * level_data_ptr;
const struct level_data * level_data_ptr_prev;

// Map decompression buffers.
uint16_t map_column[32]; // one contiguous column
uint16_t map_row[2][32]; // two contiguous rows: left and right rows (2x32)

const uint8_t * map_current;
uint16_t map_extent_x;
uint16_t map_extent_y;
uint16_t map_extent_tiles_x;
uint16_t map_extent_tiles_y;

uint16_t map_extent_tiles_x_shiftcount; // converted into amount of shifts. 16 being 4, 32 being 5, 64 being 6, 128 being 7

const uint16_t * map_lut;
const uint8_t * map_lut_col;

// Collision buffer decompresses here for speed and editability
uint8_t map_collision_buf[64*64]; // 4KB // There is no speed benefit from making this 16-bit wide

// Camera/background scroll
ZP union pos_bgscroll bg_scroll_x;
ZP union pos_bgscroll bg_scroll_y;
union pos_bgscroll bg_scroll_x_prev;
union pos_bgscroll bg_scroll_y_prev;
ZP union pos_bgscroll bg_scroll_y_mod;

union pos_bgscroll bg_scroll_x_saved;
union pos_bgscroll bg_scroll_y_saved;

union pos_bgscroll bg_scroll_x_bounds_min;
union pos_bgscroll bg_scroll_y_bounds_min;
union pos_bgscroll bg_scroll_x_bounds_max;
union pos_bgscroll bg_scroll_y_bounds_max;

uint16_t bg_scroll_use_interpolation;
uint16_t bg_scroll_x_at_final;
uint16_t bg_scroll_y_at_final;
uint16_t bg_scroll_suppress_interpolation_state_change;

// Background tile anims
// These are handled separately compared to normal DMA
// to make them possible to run on odd frames.
uint16_t ani_bg_frame_water;
uint16_t ani_bg_row_water;
uint8_t * ani_bg_addr_water;
uint16_t ani_bg_dest_water;
ZP uint16_t ani_bg_water_dma_ready;

// 64px dedicated section is updated in one go. has to go to the odd frame NMI DMAs.
uint16_t ani_bg_frame_tallbg; // the 2KB sheet
uint8_t * ani_bg_addr_tallbg;
uint16_t ani_bg_dest_tallbg;
ZP uint16_t ani_bg_tallbg_dma_ready;

// Fixed sprite tile anims
uint16_t ani_bg_frame_coin;
uint8_t * ani_bg_addr_coin;

// Palette anims, non-HDMA
uint16_t pal_ani_entries[8][2]; // Just enough for the magic circle
uint16_t pal_ani_sel;

// Shadow buffers
NEAR union oam_buffer shadow_oam;

union cgram_full shadow_cgram; // 256 palette entries, 2 bytes wide each
