#include <stdint.h> // DO NOT REMOVE THIS

#include "consts.h"
#include "defs_objects.h"
#include "defs_structs.h"

#include "vars_extern.h"

// For SRAM management
extern const uint8_t const_sram_verify_str[];
extern uint8_t sram_available_slots;

// For DMA/HDMA copies
extern const uint32_t const_zero;
extern const uint8_t const_sprite_offscreen;
extern const uint8_t const_hdma_tm_msgbox[];
extern const uint16_t const_ui_textadvance_tilemapentries[];

// System general shadows and variables
ZP extern uint16_t system_in_vblank;

ZP extern uint16_t system_current_routine;
ZP extern uint16_t system_target_routine;

ZP extern uint32_t system_frames_elapsed;

ZP extern uint8_t shadow_stat77;
ZP extern uint8_t shadow_inidisp;

ZP extern uint16_t system_nmis_counted;

// Object system
ZP extern uint16_t obj_first_available;
extern struct game_object objects[OBJ_MAX_COUNT];

extern uint16_t obj_delete_queue[OBJ_MAX_COUNT];
extern uint16_t obj_delete_queue_count;

extern uint16_t obj_player_index;
extern uint16_t obj_active_count;

extern uint16_t obj_process_count;
extern uint16_t obj_next_uid;

// Enemy data that shouldn't be in the object area
extern uint16_t obj_enemies_defeated;
extern uint16_t obj_enemies_target_count;
extern uint16_t obj_enemies_max_count;

// Player only data that shouldn't be in the object area
extern uint16_t obj_player_attack_interval;
extern uint16_t obj_player_prev_facing;
extern uint8_t * obj_player_prev_sprframe;

// Hitbox data is not done due to stale cache issues.
extern uint16_t hitbox_count_player;
extern uint16_t hitbox_count_enemy;
extern uint16_t hitbox_count_player_shadow;
extern uint16_t hitbox_count_enemy_shadow;

// Event flags
extern uint8_t event_flags_global[EVENT_FLAG_GLOBAL_MAX];
extern uint8_t event_flags_local[EVENT_FLAG_LOCAL_MAX];

extern int16_t event_interaction_x;
extern int16_t event_interaction_y;

extern uint16_t event_in_combat;
extern uint16_t event_in_combat_shadow;

extern uint16_t event_tutorial_shown;

// Blocking colliders data
extern struct tile_xy blocker_list[OBJ_MAX_COUNT];
extern uint16_t blocker_active_count;
extern uint16_t blocker_build_count;
extern uint16_t blocker_build_count_shadow; 

// Random numbers
extern int8_t rand_array[3];
extern uint16_t rand_seeded;

// DMA system
extern NEAR struct dma_entry dma_queue[DMA_QUEUE_MAX_ENTRIES];
extern uint16_t dma_queue_count;
extern uint16_t dma_queue_length;

// Input system
extern uint16_t input_pad0;
extern uint16_t input_pad0_new;
//extern uint16_t input_pad1;
//extern uint16_t input_pad1_new;

// Map system
extern const struct level_data * level_data_ptr;

// Map decompression buffers.

extern uint16_t map_column[32]; // one contiguous column
extern uint16_t map_row[2][32]; // two contiguous rows: left and right rows (2x32)

extern const uint8_t * map_current;
extern uint16_t map_extent_x;
extern uint16_t map_extent_y;
extern uint16_t map_extent_tiles_x;
extern uint16_t map_extent_tiles_y;

extern const uint16_t * map_lut;
extern const uint8_t * map_lut_col;

// Camera/background scroll
ZP extern union pos_bgscroll bg_scroll_x;
ZP extern union pos_bgscroll bg_scroll_y;
extern union pos_bgscroll bg_scroll_x_prev;
extern union pos_bgscroll bg_scroll_y_prev;
ZP extern union pos_bgscroll bg_scroll_y_mod;

extern union pos_bgscroll bg_scroll_x_saved;
extern union pos_bgscroll bg_scroll_y_saved;

extern union pos_bgscroll bg_scroll_x_bounds_min;
extern union pos_bgscroll bg_scroll_y_bounds_min;
extern union pos_bgscroll bg_scroll_x_bounds_max;
extern union pos_bgscroll bg_scroll_y_bounds_max;

extern uint16_t bg_scroll_use_interpolation;
extern uint16_t bg_scroll_x_at_final;
extern uint16_t bg_scroll_y_at_final;
extern uint16_t bg_scroll_suppress_interpolation_state_change;

// Background tile anims
// Water animations are updated per 512byte row
extern uint16_t ani_bg_frame_water; // the 2KB sheet
extern uint16_t ani_bg_row_water; // the 512 byte row section
extern uint8_t * ani_bg_addr_water;
extern uint16_t ani_bg_dest_water;
ZP extern uint16_t ani_bg_water_dma_ready;

// 64px dedicated section is updated in one go. has to go to the odd frame NMI DMAs.
extern uint16_t ani_bg_frame_tallbg; // the 2KB sheet
extern uint8_t * ani_bg_addr_tallbg;
extern uint16_t ani_bg_dest_tallbg;
ZP extern uint16_t ani_bg_tallbg_dma_ready;

// Fixed sprite tile anims
extern uint16_t ani_bg_frame_coin;
extern uint8_t * ani_bg_addr_coin;

// Palette anims, non-HDMA
extern uint16_t pal_ani_entries[8][2]; // Just enough for the magic circle
extern uint16_t pal_ani_sel;

// UI
extern uint16_t system_ui_in_bg2;

// rows x height, tilemap buffers in WRAM
//extern uint16_t ui_tilemap_2bpp[26][32];
//extern uint16_t ui_tilemap_4bpp[26][32];

extern uint16_t ui_force_update;
extern int32_t ui_cached_hp;
extern int32_t ui_cached_hp_max;
extern uint32_t ui_cached_money;
extern uint16_t ui_cached_enemy_counter;

extern uint16_t ui_hp_gauge[28];
extern uint16_t ui_money_counter[11];
extern uint16_t ui_enemy_counter[9];

extern uint16_t ui_level_status[5];

extern uint16_t ui_show_message_string_fixedwidth[4][32]; // 6x32 (30 + 2 padding)
extern uint8_t ui_show_message_string[31]; // 30 characters + null terminator
extern uint16_t ui_show_message_ttl;
extern uint16_t ui_show_message_cleared;
extern uint16_t ui_show_message_page;
extern uint8_t * ui_show_message_page_ptr_init;
extern uint8_t * ui_show_message_page_ptr;

extern uint16_t ui_show_message_border[6][32]; // textbox borders. Top 2 rows, middle 2 repeated rows, bottom 2 rows

// Sprite system
extern uint16_t spr_sprite_count; // Rendered sprites this frame
extern uint16_t spr_sprite_count_prev; // previous

extern uint16_t spr_vram_slots[128]; // VRAM slots in sprite page

extern uint16_t spr_front_count; // Rendered non-UI unsorted front-forced sprites this frame
extern struct spr_queue_entry spr_queue_front[128];
extern uint16_t spr_back_count; // Rendered non-UI unsorted back-forced sprites this frame (e.g. shadows)
extern struct spr_queue_entry spr_queue_back[128];

extern uint16_t spr_normal_count;
NEAR extern uint8_t spr_depth_count[257]; // Count of sprites on each depth line
extern struct spr_queue_entry spr_queue_normal[128]; // depth sorted sprite entries

// Shadow buffers
NEAR extern union oam_buffer shadow_oam;
extern union oam_buffer shadow_oam_copy; // copied during UI open so there's always a full copy ready to use

extern union cgram_full shadow_cgram; // 256 palette entries, 2 bytes wide each

// HDMA table for palettes and other purposes
// 6 channels usable for HDMA, so 6 arrays each
extern struct hdma_indirect_table_entry hdma_indirect_tables[2][8];
extern uint16_t hdma_indirect_data[2][224][2];

// Sound system
extern uint16_t snd_footstep_timeout;
extern uint16_t snd_punch_timeout;
extern uint16_t snd_flame_active;
extern uint16_t snd_flame_playing;
extern uint16_t snd_firecrackle_timeout;
