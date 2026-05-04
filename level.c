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

    // Load the actual map data - must be done after the player is instantiated first
    // so it knows what tilemap to load. Also specify the metatile LUT.
    map_load(
        level->map_cells, 
        level->map_lut, 
        level->map_lut_col);

    // Instantiate enemies
    obj_instantiate_spawners(level->spawner_ptr);
    obj_instantiate_interactables(level->interactable_ptr);

    // initialize global DMA tile animation
    // TODO: currently hardcoded. In the future, pointers may be part of map data.
    ani_bg_addr_water = (uint8_t *)&data_bg_dungeon_anim_water;
    ani_bg_addr_coin = (uint8_t *)&data_sprite_drop_coin;

    level_load_tileset_and_palette(level); // Must do before making palette calcs
    ani_pal_precalc_entries();
    ani_pal_hdma_setup();

    return;
}

/*
    Reload level tileset and palette, when changing screens or video modes
*/
void level_load_tileset_and_palette(const struct level_data * level)
{
    // Copy the ROM palette into shadow
    dma_copy_to_wram((uint32_t)level->tileset_palette, (uint32_t)&shadow_cgram, 512);
    
    // Copy the background graphics into VRAM
    LZ4_UnpackToVRAM(level->tileset_tiles_lz4, TILEDATA_ADDR_GAME_MAP);

    // Copy the UI graphics into VRAM
    LZ4_UnpackToVRAM((void *)&data_ui_fixed_4bpp_lz4, TILEDATA_ADDR_GAME_UI_4BPP);
    LZ4_UnpackToVRAM((void *)&data_ui_fixed_2bpp_lz4, TILEDATA_ADDR_GAME_UI_2BPP);

    // flush the BG1 tilemap with the correct null tiles
    #if VBCC_ASM == 1
        REG_VMAIN = VRAM_INCLOW;
        REG_VMADDLH = TILEMAP_ADDR_GAME_UI_4BPP;

        __asm(
            "\ta8\n"
            "\tsep #$20\n"

            "\tldx #256\n"
            "\tstx r0\n"

            "\tlda #$08\n"
            "\tsta $4300\n"
            
            "\tldx #<r0\n"
            "\tstx $4302\n"
            "\tlda #^r0\n"
            "\tsta $4304\n"

            "\tldx #1024\n"
            "\tstx $4305\n"

            "\tlda #$18\n"
            "\tsta $4301\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );

        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_GAME_UI_4BPP;

        __asm(
            "\ta8\n"
            "\tsep #$20\n"

            "\tlda #$08\n"
            "\tsta $4300\n"
            
            "\tldx #<r0+1\n"
            "\tstx $4302\n"
            "\tlda #^r0\n"
            "\tsta $4304\n"

            "\tldx #1024\n"
            "\tstx $4305\n"

            "\tlda #$19\n"
            "\tsta $4301\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_GAME_UI_4BPP;

        for (int i = 0; i < 1024; i++)
        {
            REG_VMDATALH = 256;
        }
    #endif

    // Repeat for BG3
    #if VBCC_ASM == 1
        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_GAME_UI_2BPP;

        __asm(
            "\ta8\n"
            "\tsep #$20\n"

            "\tldx #$00000\n"
            "\tstx r0\n"

            "\tlda #$09\n"
            "\tsta $4300\n"
            
            "\tldx #<r0\n"
            "\tstx $4302\n"
            "\tlda #^r0\n"
            "\tsta $4304\n"

            "\tldx #2048\n"
            "\tstx $4305\n"

            "\tlda #$18\n"
            "\tsta $4301\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_GAME_UI_2BPP;

        for (int i = 0; i < 1024; i++)
        {
            REG_VMDATALH = 0;
        }
    #endif

    return;
}
