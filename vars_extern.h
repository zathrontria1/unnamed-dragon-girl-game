// Sound engine and data
#include "vars_snd.h"

// Game data (non-audio)
extern const uint8_t data_palette[];
extern const uint8_t data_palette_map_8bpp[];
extern const uint8_t data_palette_splash[];

extern const uint8_t data_bg_dungeon_lz4[]; // dungeon tiles
extern const uint8_t data_bg_dungeon_anim_water[]; // dungeon water animation tiles
extern const uint8_t data_bg_dungeon_anim_torch[]; // dungeon torch animation tiles
extern const uint8_t data_bg_map_dungeon_8bpp_lz4[]; // dungeon map, compressed

extern const uint8_t data_bg_splash[]; // loading splash
extern const uint8_t data_tilemap_splash[]; // loading splash tilemap

extern const uint8_t data_ui_fixed_4bpp_lz4[]; // 4bpp UI
extern const uint8_t data_ui_fixed_2bpp_lz4[]; // 2bpp UI
extern const uint8_t data_ui_dynamic_hp[]; // hp bar pieces
extern const uint8_t data_ui_dynamic_textadvance[]; // text advance cursor
extern const uint8_t data_ui_dynamic_selectcursor[]; // selection cursor in text box

extern const uint8_t data_sprite_player[]; // player  
extern const uint8_t data_sprite_fixed_lz4[]; // permanently loaded effects/system
extern const uint8_t data_sprite_slime[]; // slime
extern const uint8_t data_sprite_spawn_placeholder[]; // spawning placeholder
extern const uint8_t data_sprite_drop_coin[]; // dropped coins

extern const struct obj_list_entry_interactable data_interactables_debug0[];
extern const struct obj_list_entry_interactable data_interactables_debug1[];
extern const struct obj_list_entry_interactable data_interactables_debug2[];

extern const struct obj_list_entry_spawns data_spawnlist_debug0[];
extern const struct obj_list_entry_spawns data_spawnlist_debug1[];
extern const struct obj_list_entry_spawns data_spawnlist_debug2[];

extern const struct obj_list_entry_spawners data_spawners_debug0[];
extern const struct obj_list_entry_spawners data_spawners_debug1[];
extern const struct obj_list_entry_spawners data_spawners_debug2[];

extern const int32_t data_sine_1[];
extern const int32_t data_cosine_1[];
extern const uint32_t data_pow_2[];

// Level data
extern const uint8_t data_map_debug0[];
extern const uint8_t data_map_debug1[];
extern const uint8_t data_map_debug2[];

extern const struct level_data data_level_test_0;
extern const struct level_data data_level_test_1;
extern const struct level_data data_level_test_2;

// The lookup table for maps
extern const uint16_t data_map_lut_dungeon[1024];
extern const uint8_t data_map_lut_dungeon_col[256];

// Metasprite lists
extern const struct spr_metaspr_definition data_metaspr_door_ns[];
extern const struct spr_metaspr_definition data_metaspr_door_ew[];

