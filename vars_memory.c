#include <stdint.h>

#include "vars.h"

#include "maps/map_lut_dungeon.h"
#include "maps/map_lut_dungeon_col.h"
#include "maps/map_debug0.h"
#include "data_tables.h"
#include "data_tables_lut.h"
#include "data_metaspr_list.h"

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

ZP uint8_t shadow_stat77;
ZP uint8_t shadow_inidisp;

ZP uint16_t system_nmis_counted;

// Event flags
uint8_t event_flags_global[EVENT_FLAG_GLOBAL_MAX];
uint8_t event_flags_local[EVENT_FLAG_LOCAL_MAX];

int16_t event_interaction_x;
int16_t event_interaction_y;

uint16_t event_in_combat;
uint16_t event_in_combat_shadow;

uint16_t event_tutorial_shown;

// Blocking colliders data
struct tile_xy blocker_list[OBJ_MAX_COUNT];
uint16_t blocker_active_count;
uint16_t blocker_build_count;
uint16_t blocker_build_count_shadow; 

// Object system
struct game_object objects[OBJ_MAX_COUNT];

uint16_t obj_delete_queue[OBJ_MAX_COUNT];
uint16_t obj_delete_queue_count;

uint16_t obj_player_index;
uint16_t obj_active_count;

uint16_t obj_process_count;
uint16_t obj_next_uid;

// Enemy data that shouldn't be in the object area
uint16_t obj_enemies_defeated;
uint16_t obj_enemies_target_count;
uint16_t obj_enemies_max_count;

// Player only data that shouldn't be in the object area
uint16_t obj_player_attack_interval;
uint16_t obj_player_prev_facing;
uint8_t * obj_player_prev_sprframe;

// Hitbox data is not done due to stale cache issues.
uint16_t hitbox_count_player;
uint16_t hitbox_count_enemy;
uint16_t hitbox_count_player_shadow;
uint16_t hitbox_count_enemy_shadow;

// Random numbers
int8_t rand_array[3];
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

// Map decompression buffers.
uint16_t map_column[32]; // one contiguous column
uint16_t map_row[2][32]; // two contiguous rows: left and right rows (2x32)

const uint8_t * map_current;
uint16_t map_extent_x;
uint16_t map_extent_y;
uint16_t map_extent_tiles_x;
uint16_t map_extent_tiles_y;

const uint16_t * map_lut;
const uint8_t * map_lut_col;

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

// UI
uint16_t system_ui_in_bg2;

uint16_t ui_force_update;
int32_t ui_cached_hp;
int32_t ui_cached_hp_max;
uint32_t ui_cached_money;
uint16_t ui_cached_enemy_counter;

uint16_t ui_hp_gauge[28];
uint16_t ui_money_counter[11];
uint16_t ui_enemy_counter[9];

uint16_t ui_level_status[5];

uint16_t ui_show_message_string_fixedwidth[4][32]; // 4x32 (30 + 2 padding)
uint8_t ui_show_message_string[31]; // 30 characters + null terminator
uint16_t ui_show_message_ttl;
uint16_t ui_show_message_cleared;
uint16_t ui_show_message_page;
uint8_t * ui_show_message_page_ptr_init;
uint8_t * ui_show_message_page_ptr;

uint16_t ui_show_message_border[6][32]; // textbox borders. Top 2 rows, middle 2 repeated rows, bottom 2 rows

// Sprite system
uint16_t spr_sprite_count; // Rendered sprites this frame
uint16_t spr_sprite_count_prev; // previous

uint16_t spr_vram_slots[128];

uint16_t spr_front_count; // Rendered non-UI unsorted front-forced sprites this frame
struct spr_queue_entry spr_queue_front[128];
uint16_t spr_back_count; // Rendered non-UI unsorted back-forced sprites this frame (e.g. background impostors and shadows)
struct spr_queue_entry spr_queue_back[128];

uint16_t spr_normal_count;
NEAR uint8_t spr_depth_count[257]; // Count of sprites on each depth line
struct spr_queue_entry spr_queue_normal[128]; // depth sorted sprite entries

// Shadow buffers
NEAR union oam_buffer shadow_oam;
union oam_buffer shadow_oam_copy; // copied during UI open so there's always a full copy ready to use

union cgram_full shadow_cgram; // 256 palette entries, 2 bytes wide each

// HDMA table for palettes and other purposes
// 6 channels usable for HDMA, so 6 arrays each
struct hdma_indirect_table_entry hdma_indirect_tables[2][8];
uint16_t hdma_indirect_data[2][224][2];

// Sound system
uint16_t snd_footstep_timeout;
uint16_t snd_punch_timeout;
uint16_t snd_flame_active;
uint16_t snd_flame_playing;
uint16_t snd_firecrackle_timeout;