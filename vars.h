#include <stdint.h>
#include <stdbool.h> // DO NOT REMOVE THIS

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
ZP extern uint32_t system_frames_lag;
extern uint16_t system_dont_count_lag; // set to 1 to not increment the lag counter during e.g. creating a text box

ZP extern uint8_t shadow_stat77;

ZP extern uint8_t shadow_inidisp;
ZP extern int8_t shadow_inidisp_change;

ZP extern uint8_t shadow_mosaic;

ZP extern uint8_t shadow_cgwsub;
ZP extern uint8_t shadow_cgadsub;
ZP extern uint8_t shadow_coldata_r;
ZP extern uint8_t shadow_coldata_g;
ZP extern uint8_t shadow_coldata_b;

ZP extern uint16_t system_nmis_counted;
ZP extern uint16_t system_use_alternate_nmi;

ZP extern uint16_t system_game_paused;

ZP extern void * system_loop_func_ptr;

ZP extern struct game_object * obj_player_pointer;

// Object system
ZP extern uint16_t obj_first_available;
extern struct game_object obj_general[OBJ_GENERAL_MAX_COUNT];

extern uint16_t obj_delete_queue[OBJ_GENERAL_MAX_COUNT];
extern uint16_t obj_delete_queue_count;

extern int16_t obj_player_index;
extern uint16_t obj_active_count;

extern uint16_t obj_next_uid;

// Enemy data that shouldn't be in the object area
extern uint16_t obj_enemies_defeated;
extern uint16_t obj_enemies_target_count;
extern uint16_t obj_enemies_max_count;

// Player only data that shouldn't be in the object area
extern uint16_t obj_player_attack_interval;
extern uint16_t obj_player_prev_facing;
extern uint8_t * obj_player_prev_sprframe;
extern uint16_t obj_player_active_fireballs;

extern uint16_t obj_player_health_regen_delay;
extern uint16_t obj_player_health_regen_interval;
extern uint16_t obj_player_health_regen_value;
extern uint16_t obj_player_health_regen_limit;

extern uint16_t obj_player_recovery_drop_pity;

extern uint16_t obj_player_upgrades_bought_hp;
extern uint16_t obj_player_upgrades_bought_attack;
extern uint16_t obj_player_upgrades_bought_defense;

extern uint32_t obj_player_upgrades_cost_hp;
extern uint32_t obj_player_upgrades_cost_attack;
extern uint32_t obj_player_upgrades_cost_defense;

// Hitbox data
ZP extern uint16_t obj_hitbox_player_first_available;
extern struct game_object obj_hitbox_player[OBJ_PLAYERHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_player_delete_queue[OBJ_PLAYERHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_player_delete_queue_count;
extern uint16_t obj_hitbox_count_player;

ZP extern uint16_t obj_hitbox_enemy_first_available;
extern struct game_object obj_hitbox_enemy[OBJ_ENEMYHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_enemy_delete_queue[OBJ_ENEMYHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_enemy_delete_queue_count;
extern uint16_t obj_hitbox_count_enemy;

// Event flags
extern uint8_t event_flags_global[EVENT_FLAG_GLOBAL_MAX];
extern uint8_t event_flags_local[EVENT_FLAG_LOCAL_MAX];

extern int16_t event_interaction_x;
extern int16_t event_interaction_y;

extern uint16_t event_in_combat;
extern uint16_t event_in_combat_shadow;

extern uint16_t event_tutorial_shown;

// Random numbers
extern ZP int8_t rand_array[3];
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
extern const struct level_data * level_data_ptr_prev;

// Map decompression buffers.
extern uint16_t map_column[32]; // one contiguous column
extern uint16_t map_row[2][32]; // two contiguous rows: left and right rows (2x32)

extern const uint8_t * map_current;
extern uint16_t map_extent_x;
extern uint16_t map_extent_y;
extern uint16_t map_extent_tiles_x;
extern uint16_t map_extent_tiles_y;

extern uint16_t map_extent_tiles_x_shiftcount; // converted into amount of shifts. 16 being 4, 32 being 5, 64 being 6, 128 being 7

extern const uint16_t * map_lut;
extern const uint8_t * map_lut_col;

// Collision buffer decompresses here for speed and editability
extern uint8_t map_collision_buf[64*64]; // 4KB // There is no speed benefit from making this 16-bit wide

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

// Fixed sprite tile anims
extern uint16_t ani_bg_frame_coin;
extern uint8_t * ani_bg_addr_coin;

// Palette anims, non-HDMA
extern uint16_t pal_ani_entries[8][2]; // Just enough for the magic circle
extern uint16_t pal_ani_sel;

// Shadow buffers
NEAR extern union oam_buffer shadow_oam;

extern union cgram_full shadow_cgram; // 256 palette entries, 2 bytes wide each
