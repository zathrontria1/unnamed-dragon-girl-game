// Sound engine and data
#include "vars_extern_snd.h"
#include "vars_extern_cutscenes.h"

extern const struct enemy_data data_enemy_stats_slime;
extern const struct enemy_data data_enemy_stats_lizardman;
extern const struct enemy_data data_enemy_stats_lizardman_archer;
extern const struct enemy_data data_enemy_stats_lizardman_lilsis;
extern const struct enemy_data data_enemy_stats_boss_0;

// Game data (non-audio)
extern const uint8_t data_palette_blank[];

extern const uint8_t data_palette_ui[];
extern const uint8_t data_palette_dungeon_0[];
extern const uint8_t data_palette_dungeon_1[];
extern const uint8_t data_palette_dungeon_2[];

extern const uint8_t data_palette_player[];
extern const uint8_t data_palette_player_portrait[];
extern const uint8_t data_palette_common_0[];
extern const uint8_t data_palette_common_1[];
extern const uint8_t data_palette_boss_0[];

extern const uint8_t data_palette_cycle_fire[];

extern const uint8_t data_palette_map_0_8bpp[];
extern const uint8_t data_palette_map_1_8bpp[];

extern const uint8_t data_palette_splash[];
extern const uint8_t data_palette_error[];

// Backgrounds and their tilemap data may be compressed with LZ4. 
// _lz4 = lz4 compressed
extern const uint8_t data_bg_dungeon_lz4[]; // dungeon tiles
extern const uint8_t data_bg_dungeon_anim_water_lz4[]; // dungeon water animation tiles
extern const uint8_t data_bg_dungeon_anim_torch_lz4[]; // dungeon torch animation tiles
extern const uint8_t data_bg_map_dungeon_0_8bpp_lz4[]; // dungeon map, compressed
extern const uint8_t data_bg_map_dungeon_1_8bpp_lz4[]; // dungeon map, compressed

extern const uint8_t data_bg_error_back_lz4[]; // error general back
extern const uint8_t data_tilemap_error_back_lz4[];

extern const uint8_t data_ui_fixed_4bpp_lz4[]; // 4bpp UI
extern const uint8_t data_ui_fixed_2bpp_lz4[]; // 2bpp UI
extern const uint8_t data_ui_dynamic_hp[]; // hp bar pieces
extern const uint8_t data_ui_dynamic_textadvance[]; // text advance cursor
extern const uint8_t data_ui_dynamic_selectcursor[]; // selection cursor in text box
extern const uint8_t data_ui_vwf[]; // VWF glyphs

// Sprites may be compressed with tile deduplication or LZ4. 
// _dd = deduped; will also have a LUT (_lut)
// _lz4 = lz4 compressed
extern const uint8_t data_spr_player_dd[]; // player deduplicated tiles
extern const uint16_t data_spr_player_lut[]; // player tile lookup

extern const uint8_t data_spr_player_portrait_lz4[]; // player portrait

extern const uint8_t data_spr_fixed_lz4[]; // permanently loaded effects/system

extern const uint8_t data_spr_slime[]; // slime
extern const uint8_t data_spr_lizardman[]; // lizardman
extern const uint8_t data_spr_spawn_placeholder[]; // spawning placeholder
extern const uint8_t data_spr_drop_coin[]; // dropped coins

extern const uint8_t data_spr_boss_placeholder_dd[]; // boss
extern const uint16_t data_spr_boss_placeholder_lut[]; // boss
extern const uint8_t data_spr_boss_placeholder_addon_attack1[]; // boss attack 1
extern const uint8_t data_spr_boss_placeholder_hands[]; /// boss hands

// Tables
extern const NEAR int32_t data_sine_1[];
extern const NEAR int32_t data_cosine_1[];
extern const NEAR uint32_t data_pow_2[];

extern const uint32_t data_upgrade_costs[];

// Level sub-data
extern const struct level_palette_list data_palette_list_0;

extern const struct obj_list_entry_interactable data_interactables_debug0[];
extern const struct obj_list_entry_interactable data_interactables_debug1[];
extern const struct obj_list_entry_interactable data_interactables_debug2[];

extern const struct obj_list_entry_spawns data_spawnlist_debug0[];
extern const struct obj_list_entry_spawns data_spawnlist_debug1[];
extern const struct obj_list_entry_spawns data_spawnlist_debug2[];
extern const struct obj_list_entry_spawns data_spawnlist_boss[];

extern const struct obj_list_entry_spawners data_spawners_debug0[];
extern const struct obj_list_entry_spawners data_spawners_debug1[];
extern const struct obj_list_entry_spawners data_spawners_debug2[];

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
extern const struct spr_metaspr_definition data_metaspr_level_warp_closed[];

extern const struct spr_metaspr_definition data_metaspr_door_ns_open[];
extern const struct spr_metaspr_definition data_metaspr_door_ns_closed[];
extern const struct spr_metaspr_definition data_metaspr_door_ew_open[];
extern const struct spr_metaspr_definition data_metaspr_door_ew_open_flip[];
extern const struct spr_metaspr_definition data_metaspr_door_ew_closed[];

extern const struct spr_metaspr_definition data_metaspr_shadow_64x16[];
extern const struct spr_metaspr_definition data_metaspr_shadow_32x16[];

extern const struct spr_metaspr_definition data_metaspr_boss_generic_64x96[];
extern const struct spr_metaspr_definition data_metaspr_boss_generic_64x96_hflip[];

extern const struct spr_metaspr_definition data_metaspr_boss_generic_hands_l[];
extern const struct spr_metaspr_definition data_metaspr_boss_generic_hands_r[];
extern const struct spr_metaspr_definition data_metaspr_boss_generic_hands_hflip_l[];
extern const struct spr_metaspr_definition data_metaspr_boss_generic_hands_hflip_r[];

extern const struct spr_metaspr_definition data_metaspr_shadow_hands[];
extern const struct spr_metaspr_definition data_metaspr_shadow_hands_hflip[];
