#include <snes/console.h>
#include <stdint.h>

#include "vars.h"

#include "level.h"
#include "map.h"
#include "obj.h"
#include "ani_bg.h"
#include "ani_pal.h"
#include "ani_pal_hdma.h"

#include "dma.h"
#include "lz4.h"

// All functions in level.c expect that a valid level pointer is set.

/*
    Load level data in pointer

    Split into two parts so part of it can be done while the screen is turned on
*/
void level_load(const struct level_data * level)
{
    // Instantiate player
    obj_player_index = obj_instantiate(
        OBJID_PLAYER, 
        level->player_start_x, 
        level->player_start_y, 
        0); 
    obj_player_prev_facing = FACING_DOWN;

    // Instantiate enemies
    obj_instantiate_spawners(level->spawner_ptr);
    obj_instantiate_interactables(level->interactable_ptr);
    
    // initialize global DMA tile animation
    // TODO: currently hardcoded. In the future, pointers may be part of map data.
    ani_bg_addr_water = (uint8_t *)&data_bg_dungeon_anim_water;
    ani_bg_addr_coin = (uint8_t *)&data_sprite_drop_coin;
    
    level_load_graphics(level); // Now no longer hits VRAM
    level_load_palette(level); // Must do before making palette calcs
    ani_pal_precalc_entries();
    ani_pal_hdma_setup();

    return;
}

/*
    Map graphics that require fblank go here
*/
void level_load_graphics(const struct level_data * level)
{
    // Load the actual map data - must be done after the player is instantiated first
    // so it knows what tilemap to load. Also specify the metatile LUT.
    map_load(
        level->map_cells, 
        level->map_lut, 
        level->map_lut_col);

    level_load_tileset(level); 
    
    return;
}

/*
    Reload level tileset, when changing screens or video modes
*/
void level_load_tileset(const struct level_data * level)
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
void level_load_palette(const struct level_data * level)
{
    // Copy the ROM palette into shadow
    dma_copy_to_wram((uint32_t)level->tileset_palette, (uint32_t)&shadow_cgram, 512);

    return;
}
