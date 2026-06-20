#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "vars.h"

#include "loop.h"
#include "loop_cutscene.h"

#include "level.h"

#include "dma.h"

#include "ani.h"
#include "ani_bg.h"
#include "ani_pal.h"

#include "gfx.h"

#include "interrupt.h"
#include "system.h"

#include "hdma.h"
#include "snd.h"
#include "consts_snd.h"

#include "main.h"

struct cutscene_data * cs_current; // Current cutscene frame.
uint16_t cs_timer; // Current cutscene remaining time before auto advance

bool cs_use_second_frame; // Whether to use the alternate frame. Also determines which section preload

/* 
    Preload sections can be thought as follows
    0 = first 4KB
    1 = next 4KB
    2, 3, 4 = ditto until 20KB has been reached
    5 = tilemap

    if 6 or above, don't preload
*/
uint16_t cs_preload_subsection; // Which preload section we're at

void CsEngine_Loop()
{
    System_Init_DisplaySettings(system_target_routine);
    System_Init_TilemapSettings(system_target_routine);

    bool skip = false;

    if (cs_timer > 0)
    {
        cs_timer--;
    }

    if (System_CheckKey(KEY_A))
    {
        cs_timer = 0;

        SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
    }

    if (System_CheckKey(KEY_START))
    {
        skip = true;

        SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
    }

    // Perform a preload if needed
    if ((uint32_t)((cs_current+1)->frame) != 0xffffffff)
    {
        if (cs_preload_subsection < 2)
        {
            CsEngine_PreloadNextFrame();
        }
    }
    else
    {
        cs_preload_subsection = 2;
    }
    
    if ((cs_timer == 0) || skip)
    {
        if (cs_preload_subsection < 2)
        {
            if (!skip)
            {
                return;
            }
        }

        // Advance the current cutscene frame
        cs_current++;

        if (((uint32_t)(cs_current->frame) == 0xffffffff) || skip)
        {
            // Cutscene has ended.
            System_DisableInterrupts();

            // Perform level load stuff
            LevelSystem_LoadLevel(level_data_ptr); // non-VRAM hitting parts here

            shadow_inidisp_change = 0;
            gfx_mosaic_change = 0;
            system_use_alternate_nmi = 0;

            while (shadow_inidisp != 0x00)
            {
                while ((REG_HVBJOY & VBL_READY) != VBL_READY)
                {
                    ;
                }

                REG_INIDISP = shadow_inidisp;

                REG_MOSAIC = shadow_mosaic;

                shadow_mosaic = (((0x0f - shadow_inidisp) << 4) | 0x01);

                while ((REG_HVBJOY & VBL_READY) == VBL_READY)
                {
                    ;
                }

                shadow_inidisp -= 1;
            }

            shadow_mosaic = 0x00;

            REG_MOSAIC = shadow_mosaic;
            REG_INIDISP = 0x8f;
            shadow_inidisp = 0;

            // Screen is forced blank again. Do anything that touches PPU regs here now

            // For now don't touch anything here
            // DMA graphics in its entirety
            DmaSystem_CopyToVram(0x007f0000, 0x0000, 0);

            // Initialize global DMA tile animation
            // TODO: currently hardcoded. In the future, pointers may be part of map data.
            buf_player_prev_frame = 0xffff; // Write an invalid frame here

            AniSystem_BgTile_Setup((uint8_t *)&data_bg_dungeon_anim_water_lz4, (uint8_t *)&data_bg_dungeon_anim_torch_lz4);

            // Finish initializing graphics
            System_Init_Graphics();

            // Run one frame of partial game logic to draw sprites
            Loop_Game_Partial();

            HdmaEngine_EnableHdma();

            SoundInterface_PlayMusic(); 

            system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_FADEIN);
            system_target_routine = ROUTINE_GAMELOOP;

            System_Init_TilemapSettings(system_target_routine);
            System_Init_DisplaySettings(system_target_routine);

            system_suppress_odd_transfers = false;
            
            System_EnableInterrupts();

            return;
        }
        else
        {
            CsEngine_StartCutscene();
        }
    }

    return;
}

/*
    Call to setup a cutscene
*/
void CsEngine_StartCutscene()
{
    if (system_current_routine == ROUTINE_CUTSCENE_INIT)
    {
        cs_use_second_frame = false;

        System_AlignToVblank();

        shadow_inidisp = 0x8f;
        REG_INIDISP = 0x8f;

        System_DisableInterrupts();

        // Write an empty tile at the 20KB boundary of both frames.
        DmaSystem_CopyToVram((uint32_t)const_zero, 0x2800, 32);
        DmaSystem_CopyToVram((uint32_t)const_zero, 0x5800, 32);

        // Write the frame data.
        DmaSystem_CopyToVram((uint32_t)cs_current->frame, 0x0000, 20480);
        DmaSystem_CopyToVram((uint32_t)cs_current->tilemap, TILEMAP_ADDR_CS_FRAME_A, 1280);
        DmaSystem_CopyToWram((uint32_t)cs_current->palette, (uint32_t)&shadow_cgram, 256);

        // Stub out the tilemap data past entry 640 (byte 1280)
        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_CS_FRAME_A+640;
        for (int i = 640; i < 1024; i++)
        {
            REG_VMDATALH = 640;
        }
        REG_VMADDLH = TILEMAP_ADDR_CS_FRAME_B+640;
        for (int i = 640; i < 1024; i++)
        {
            REG_VMDATALH = 640;
        }

        DmaSystem_UploadCgram();

        REG_BG1VOFS = 0xdf;
        REG_BG1VOFS = 0xff;

        cs_preload_subsection = 0;
    }
    else
    {
        cs_use_second_frame ^= true;

        DmaSystem_CopyToWram((uint32_t)cs_current->palette, (uint32_t)&shadow_cgram, 256);

        cs_preload_subsection = 0;
    }

    cs_timer = cs_current->time;

    System_AlignToVblank();

    if (system_current_routine == ROUTINE_CUTSCENE_INIT)
    {
        shadow_inidisp = 0x00;
        REG_INIDISP = 0x00;
        system_current_routine = ROUTINE_FADEIN;

        system_target_routine = ROUTINE_CUTSCENE;
    }
    else
    {
        shadow_inidisp = 0x0f;
        REG_INIDISP = 0x0f;
        system_current_routine = ROUTINE_CUTSCENE;
        
        system_target_routine = ROUTINE_CUTSCENE;
    }

    shadow_hdmaen = 0x00;
    system_suppress_odd_transfers = true;

    system_loop_func_ptr = main_GetFunctionPointer(system_current_routine);

    System_EnableFblankInterrupts();

    return;
}

/*
    Call to preload next frame data
*/
void CsEngine_PreloadNextFrame()
{
    struct cutscene_data * ptr = cs_current + 1;

    uint16_t offset_vram = cs_preload_subsection * 5120; // 5120 words
    uint16_t offset = offset_vram << 1;

    uint16_t offset_tilemap_vram = cs_preload_subsection * 320; // 320 words
    uint16_t offset_tilemap = offset_tilemap_vram << 1;

    // Tile data in 10KB chunks, and tilemap data in 640b chunks
    // It fits in DMA queue system.
    if (!cs_use_second_frame)
    {
        DmaSystem_AddItemToQueue((uint8_t *)((uint32_t)ptr->frame + offset), 0x3000+offset_vram, 10240, VRAM_INCHIGH, 0);
        DmaSystem_AddItemToQueue((uint8_t *)((uint32_t)ptr->tilemap + offset_tilemap), TILEMAP_ADDR_CS_FRAME_B+offset_tilemap_vram, 640, VRAM_INCHIGH, 0);
    }
    else
    {
        DmaSystem_AddItemToQueue((uint8_t *)((uint32_t) ptr->frame + offset), 0x0000+offset_vram, 10240, VRAM_INCHIGH, 0);
        DmaSystem_AddItemToQueue((uint8_t *)((uint32_t)ptr->tilemap + offset_tilemap), TILEMAP_ADDR_CS_FRAME_A+offset_tilemap_vram, 640, VRAM_INCHIGH, 0);
    }

    cs_preload_subsection++;

    return;
}
