#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "level.h"
#include "map.h"
#include "obj.h"
#include "ani_bg.h"
#include "ani_pal.h"
#include "ani_fixedspr.h"
#include "hdma.h"

#include "dma.h"
#include "lz4.h"

#include "crash_handler.h"

const struct level_data * level_data_ptr;
const struct level_data * level_data_ptr_prev;
const struct level_data * level_data_ptr_next;

// All functions in level.c expect that a valid level pointer is set.

/**
 * @brief Loads level map layout, instantiates objects, and sets up palette/tileset buffers.
 * 
 * @param level Pointer to the level data configuration.
 * @return true if the level reuses previous VRAM contents without needing a full reload, false otherwise.
 */
bool LevelSystem_LoadLevel(const struct level_data * level)
{
    bool temp_level_reuses_vram_contents = false;

    // Instantiate player if the player isn't already instantiated
    if (obj_player_index == -1)
    {
        obj_player_index = ObjectSystem_InstantiateObject(
            OBJID_PLAYER, 
            level->player_start_x, 
            level->player_start_y, 
            0); 

        if (obj_player_index == -1)
        {
            crashhandler_error_code = CRASHHANDLER_ERROR_PLAYER_INSTANTIATION;
            System_CrashHandler();
        }

        obj_player_prev_facing = FACING_DOWN;

        obj_player_pointer = (struct game_object *)&obj_general[obj_player_index];

        obj_player_health_regen_delay = PLAYER_HEALTH_REGEN_DELAY;
        obj_player_health_regen_interval = PLAYER_HEALTH_REGEN_INTERVAL;
        obj_player_health_regen_value = PLAYER_HEALTH_REGEN_VALUE;
        obj_player_health_regen_limit = obj_general[obj_player_index].struct_data.npc_data.hp_max >> PLAYER_HEALTH_REGEN_LIMITSHIFT;

        obj_player_recovery_drop_pity = ENEMY_DROP_REC_PITY;
    }
    else
    {
        obj_player_pointer->pos.x.lh.h = level->player_start_x;
        obj_player_pointer->pos.y.lh.h = level->player_start_y;

        obj_player_pointer->pos.x.lh.l = 0;
        obj_player_pointer->pos.y.lh.l = 0;
    }

    // Load the target map now that obj_player_pointer is valid
    MapSystem_LoadMap(
        level->map_cells,
        level->map_lut,
        level->map_lut_col);
    
    // Reset local event flags
    for (int i = 0; i < 16; i++)
    {
        event_flags_local[i] = 0;
    }

    // Instantiate enemies
    if (ObjectSystem_List_InstantiateSpawners((const struct obj_list_entry_spawners*)level->spawner_ptr))
    {
        crashhandler_error_code = CRASHHANDLER_ERROR_SPAWNER_LIST_INSTANTIATION;
        System_CrashHandler();
    }

    if (ObjectSystem_List_InstantiateInteractables((const struct obj_list_entry_interactable*)level->interactable_ptr))
    {
        crashhandler_error_code = CRASHHANDLER_ERROR_INTERACTABLE_LIST_INSTANTIATION;
        System_CrashHandler();
    }

    // initialize coin DMA tile animation
    ani_fixedspr_addr_coin = (uint8_t *)&data_spr_drop_coin;

    // If the pointers point to the same thing, assume that a full reload is needed
    if (level_data_ptr_prev == level_data_ptr)
    {
        LevelSystem_LoadLevelGraphics(level); // Now no longer hits VRAM
        LevelSystem_LoadLevelPalette(level); // Must do before making palette calcs
    
        AniSystem_Pal_PrecalcPaletteChanges();
        HdmaEngine_SetupHdma();
    }
    else
    {
        // Note that these don't need to be reloaded if the graphics and palettes are the same as the last time.
        if (level->tileset_tiles_lz4 != level_data_ptr_prev->tileset_tiles_lz4)
        {
            LevelSystem_LoadLevelGraphics(level); // Now no longer hits VRAM
        }
        else
        {
            // Just load the new map cells
            /*MapSystem_LoadMap(
            level->map_cells, 
            level->map_lut, 
            level->map_lut_col);*/

            temp_level_reuses_vram_contents = true; // Flag to avoid VRAM DMA if not needed
        }

        if (level->tileset_palette != level_data_ptr_prev->tileset_palette)
        {
            LevelSystem_LoadLevelPalette(level); // Must do before making palette calcs
        
            AniSystem_Pal_PrecalcPaletteChanges();
            HdmaEngine_SetupHdma();
        }
        else
        {
            // Just update the scroll tables
            HdmaEngine_UpdateBgScrollValues();
            HdmaEngine_UpdateBgScrollValues(); // Yes, run this twice, so both tables are populated
        
            // Same for COLDATA
            HdmaEngine_UpdateColdataValues();
            HdmaEngine_UpdateColdataValues();
        }
    }

    level_data_ptr_prev = level_data_ptr;

    return temp_level_reuses_vram_contents;
}

/**
 * @brief Unpacks map metatiles and tileset graphics into Bank 7F WRAM staging areas.
 * 
 * @param level Pointer to the level data configuration.
 */
void LevelSystem_LoadLevelGraphics(const struct level_data * level)
{
    // Load the actual map data - must be done after the player is instantiated first
    // so it knows what tilemap to load. Also specify the metatile LUT.
    /*MapSystem_LoadMap(
        level->map_cells, 
        level->map_lut, 
        level->map_lut_col);*/

    LevelSystem_LoadLevelTileset(level); 
    
    return;
}

/**
 * @brief Decompresses sprite, BG map, and UI tilesets into Bank 7F WRAM.
 * 
 * @param level Pointer to the level data configuration.
 */
void LevelSystem_LoadLevelTileset(const struct level_data * level)
{
    // Copy fixed sprite graphics
    LZ4_UnpackToWRAM((void *)&data_spr_fixed_lz4, (void *)(0x007f0000 | ((uint32_t)TILEDATA_ADDR_SPRITES << 1))); 

    // Copy the background graphics into WRAM
    LZ4_UnpackToWRAM(level->tileset_tiles_lz4, (void *)(0x007f0000 | ((uint32_t)TILEDATA_ADDR_GAME_MAP << 1))); 

    // Copy the UI graphics into WRAM
    LZ4_UnpackToWRAM((void *)&data_ui_fixed_4bpp_lz4, (void *)(0x007f0000 | ((uint32_t)TILEDATA_ADDR_GAME_UI_4BPP << 1))); 
    LZ4_UnpackToWRAM((void *)&data_ui_fixed_2bpp_lz4, (void *)(0x007f0000 | ((uint32_t)TILEDATA_ADDR_GAME_UI_2BPP << 1))); 

    return;
}

/**
 * @brief Loads the level's 16 subpalettes into the CGRAM shadow buffer.
 * 
 * @param level Pointer to the level data configuration.
 */
void LevelSystem_LoadLevelPalette(const struct level_data * level)
{
    uint8_t ** pal_ptr = (uint8_t **)level->tileset_palette; // Pointer of pointers
    // Copy the ROM palette into shadow
    for (int i = 0; i < 16; i++)
    {
        uint8_t * subpal_ptr = *pal_ptr;

        AniSystem_Pal_LoadSubpalette(subpal_ptr, i);

        pal_ptr++;
    }

    return;
}

const struct level_data * const_level_pointer_table[LEVEL_ID_COUNT] = {
    (void *)&data_level_test_0,
    (void *)&data_level_test_1,
    (void *)&data_level_test_2
};
