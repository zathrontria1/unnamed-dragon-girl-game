#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "level.h"
#include "map.h"
#include "obj.h"
#include "ani_bg.h"
#include "ani_pal.h"
#include "hdma.h"

#include "dma.h"
#include "lz4.h"

// All functions in level.c expect that a valid level pointer is set.

/*
    Load level data in pointer

    Split into two parts so part of it can be done while the screen is turned on
*/
bool LevelSystem_LoadLevel(const struct level_data * level)
{
    // Instantiate player if the player isn't already instantiated

    // TODO: behaviorial differences with Calypsi here that causes the game to be unable to
    // switch levels here. If the player is outright re-initialized, it does work.

    bool temp_level_reuses_vram_contents = false;

    // Suspect: pointer errors
    if (obj_player_index == -1)
    {
        obj_player_index = obj_instantiate(
            OBJID_PLAYER, 
            level->player_start_x, 
            level->player_start_y, 
            0); 

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

    
    // Reset local event flags
    for (int i = 0; i < 256; i++)
    {
        event_flags_local[i] = 0;
    }

    // Instantiate enemies
    obj_instantiate_spawners((const struct obj_list_entry_spawners*)level->spawner_ptr);

    obj_instantiate_interactables((const struct obj_list_entry_interactable*)level->interactable_ptr);
    
    // initialize global DMA tile animation
    // TODO: currently hardcoded. In the future, pointers may be part of map data.
    LZ4_UnpackToWRAM((uint8_t *)&data_bg_dungeon_anim_water_lz4, (uint32_t)&ani_bg_strip);
    LZ4_UnpackToWRAM((uint8_t *)&data_bg_dungeon_anim_torch_lz4, (uint32_t)&ani_bg_frame);

    AniSystem_BgTile_SetStripPointer((uint8_t *)&ani_bg_strip);
    AniSystem_BgTile_SetFramePointer((uint8_t *)&ani_bg_frame);

    ani_bg_addr_coin = (uint8_t *)&data_sprite_drop_coin;

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
            MapSystem_LoadMap(
            level->map_cells, 
            level->map_lut, 
            level->map_lut_col);

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
        }
    }

    return temp_level_reuses_vram_contents;
}

/*
    Map graphics that require fblank go here
*/
void LevelSystem_LoadLevelGraphics(const struct level_data * level)
{
    // Load the actual map data - must be done after the player is instantiated first
    // so it knows what tilemap to load. Also specify the metatile LUT.
    MapSystem_LoadMap(
        level->map_cells, 
        level->map_lut, 
        level->map_lut_col);

    LevelSystem_LoadLevelTileset(level); 
    
    return;
}

/*
    Reload level tileset, when changing screens or video modes
*/
void LevelSystem_LoadLevelTileset(const struct level_data * level)
{
    // Copy fixed sprite graphics
    LZ4_UnpackToWRAM((void *)&data_sprite_fixed_lz4, ((uint32_t)0x007f0000 | ((uint32_t)TILEDATA_ADDR_SPRITES << 1))); 
    //LZ4_UnpackToVRAM((void *)&data_sprite_fixed_lz4, TILEDATA_ADDR_SPRITES);

    // Copy the background graphics into WRAM
    LZ4_UnpackToWRAM(level->tileset_tiles_lz4, ((uint32_t)0x007f0000 | ((uint32_t)TILEDATA_ADDR_GAME_MAP << 1))); 
    //LZ4_UnpackToVRAM(level->tileset_tiles_lz4, TILEDATA_ADDR_GAME_MAP);

    // Copy the UI graphics into WRAM
    LZ4_UnpackToWRAM((void *)&data_ui_fixed_4bpp_lz4, ((uint32_t)0x007f0000 | ((uint32_t)TILEDATA_ADDR_GAME_UI_4BPP << 1))); 
    //LZ4_UnpackToVRAM((void *)&data_ui_fixed_4bpp_lz4, TILEDATA_ADDR_GAME_UI_4BPP);
    LZ4_UnpackToWRAM((void *)&data_ui_fixed_2bpp_lz4, ((uint32_t)0x007f0000 | ((uint32_t)TILEDATA_ADDR_GAME_UI_2BPP << 1))); 
    //LZ4_UnpackToVRAM((void *)&data_ui_fixed_2bpp_lz4, TILEDATA_ADDR_GAME_UI_2BPP);

    return;
}

/*
    Same for the palette
*/
void LevelSystem_LoadLevelPalette(const struct level_data * level)
{
    // Copy the ROM palette into shadow
    DmaSystem_CopyToWram((uint32_t)level->tileset_palette, (uint32_t)&shadow_cgram, 512);

    return;
}
