#ifndef DATA_MAPS_H
#define DATA_MAPS_H

#include <stdint.h>
#include <stdbool.h>

#include "consts.h"
#include "defs_objects.h"
#include "defs_structs.h"
#include "vars_extern.h"
#include "data_strings.h"

// Map cell arrays and LUTs
#include "maps/map_lut_dungeon.h"
#include "maps/map_lut_dungeon_col.h"
#include "maps/map_debug0.h"
#include "maps/map_debug1.h"

// Forward declarations for level headers referenced by interactables (e.g. warps)
extern const struct level_data data_map_debug0_header;
extern const struct level_data data_map_debug1_header;
extern const struct level_data data_map_debug2_header;

// Interactable lists
const struct obj_list_entry_interactable data_map_debug0_interactables[] = 
{
    {OBJID_INTERACTABLE_SWITCH_WALL, 440, 432-8, 0},
    {OBJID_INTERACTABLE_BLOCKER_DOOR_NS, 496, 432, 0}, 
    {OBJID_INTERACTABLE_BLOCKER_DOOR_EW, 384, 512, 0}, 
    {OBJID_INTERACTABLE_BLOCKER_DOOR_EW, 624, 512, 0}, 
    {OBJID_INTERACTABLE_BLOCKER_DOOR_NS, 496, 640, 0}, 

    {OBJID_INTERACTABLE_LEVEL_WARP, 496, 32, (void *)&data_map_debug1_header}, 

#if defined(DEBUG_ALL)
    // Debug/cheat treasures for testing upgrades, money counters, and subscreens.
    {OBJID_INTERACTABLE_TREASURECHEST, 312, 320, (void *)5000l}, 
    {OBJID_INTERACTABLE_TREASURECHEST, 312, 320-32, (void *)100000l}, 
    {OBJID_INTERACTABLE_TREASURECHEST, 312-32, 320, (void *)500000l}, 
    {OBJID_INTERACTABLE_TREASURECHEST, 312+32, 320, (void *)10000000l}, 
    {OBJID_INTERACTABLE_TREASURECHEST, 312, 320+32, (void *)50000000l}, 
#endif
    {OBJID_NULL, 0, 0, 0}, 
};

const struct obj_list_entry_interactable data_map_debug1_interactables[] = 
{
    {OBJID_INTERACTABLE_SWITCH_WALL, 424, 688-8, 0},
    {OBJID_INTERACTABLE_BLOCKER_DOOR_NS, 448, 672, 0}, 
    {OBJID_INTERACTABLE_SIGN_WALL, 496, 688-8, (void *)&STR_MSG_TUTORIAL_BOSS}, 
    {OBJID_NULL, 0, 0, 0}, 
};

// Spawner lists
const struct obj_list_entry_spawners data_map_debug0_spawners[] = {
    {OBJID_SPAWNER_ENEMY, 400+15, 448+15, 224-31, 176-31, 384, 400, 256, 224, (void *)&data_spawnlist_debug0},
    {OBJID_SPAWNER_ENEMY, 16+15, 448+15, 224-31, 176-31, 0, 400, 256, 224, (void *)&data_spawnlist_debug1},
    {OBJID_SPAWNER_ENEMY, 400+15, 48+15, 224-31, 176-31, 384, 0, 256, 224, (void *)&data_spawnlist_debug0},
    {OBJID_SPAWNER_ENEMY, 400+15, 848+15, 224-31, 176-31, 384, 800, 256, 224, (void *)&data_spawnlist_debug1},
    {OBJID_NULL, 0, 0, 1, 1, 0, 0, 0, 0, 0},
};

const struct obj_list_entry_spawners data_map_debug1_spawners[] = {
    {OBJID_SPAWNER_ENEMY, 224+15, 256+15, 480-31, 400-31, 208, 208, 512, 448, (void *)&data_spawnlist_boss},
    {OBJID_NULL, 0, 0, 1, 1, 0, 0, 0, 0, 0},
};

// Level data structs
const struct level_data data_map_debug0_header = {
    504,
    504,

    (void *)&data_bg_dungeon_lz4,
    (void *)&data_palette_list_0,

    (void *)&data_map_debug0_cells,
    (void *)&data_map_lut_dungeon,
    (void *)&data_map_lut_dungeon_col,

    (void *)&data_map_debug0_spawners,
    (void *)&data_map_debug0_interactables,

    (void *)&data_bg_map_dungeon_0_8bpp_lz4,
    (void *)&data_palette_map_0_8bpp,

    (void *)&STR_LEVELNAME_DEBUG_B1F
};

const struct level_data data_map_debug1_header = {
    56,
    864,

    (void *)&data_bg_dungeon_lz4,
    (void *)&data_palette_list_0,

    (void *)&data_map_debug1_cells,
    (void *)&data_map_lut_dungeon,
    (void *)&data_map_lut_dungeon_col,

    (void *)&data_map_debug1_spawners,
    (void *)&data_map_debug1_interactables,

    (void *)&data_bg_map_dungeon_1_8bpp_lz4,
    (void *)&data_palette_map_1_8bpp,

    (void *)&STR_LEVELNAME_DEBUG_B2F
};

const struct level_data data_map_debug2_header = {
    504,
    504,

    (void *)&data_bg_dungeon_lz4,
    (void *)&data_palette_list_0,

    (void *)&data_map_debug0_cells,
    (void *)&data_map_lut_dungeon,
    (void *)&data_map_lut_dungeon_col,

    (void *)&data_map_debug0_spawners,
    (void *)&data_map_debug0_interactables,

    (void *)&data_bg_map_dungeon_0_8bpp_lz4,
    (void *)&data_palette_map_0_8bpp,

    (void *)&STR_LEVELNAME_DEBUG_B3F
};

#endif // DATA_MAPS_H
