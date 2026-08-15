#include "snes/console.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "data_strings.h"

#include "lz4.h"
#include "system.h"

#include "gfx.h"
#include "dma.h"
#include "hdma.h"
#include "ui_vwf.h"
#include "errorhandling.h"

#include "loop_title.h"
#include "main.h"
#include "loop_cutscene.h"
#include "snd.h"
#include "consts_snd.h"

#include "spr.h"
#include "vars_extern.h"

// Placeholder; this should be filled with real code sometime soon
void Title_Loop()
{
    hdma_use_gradient = 0x0000;

    system_dont_count_lag = true;

    if (shadow_brightness < (15 << 8))
    {
        while (shadow_brightness < (15 << 8))
        {
            while ((REG_HVBJOY & VBL_READY) != VBL_READY)
            {
                ;
            }

            REG_INIDISP = shadow_brightness >> 8;

            while ((REG_HVBJOY & VBL_READY) == VBL_READY)
            {
                ;
            }

            shadow_brightness += (64 * V_MUL);
        }
    }

    // Set up the display for the title screen
    //System_Init_TilemapSettings(ROUTINE_TITLE);
    //System_Init_DisplaySettings(ROUTINE_TITLE);

    REG_OBSEL = OBJ_SIZE16_L32|3; // Enable sprite layer, needed for the title options

    // Blink the PRESS START text on the title screen
    if (system_time_subframe % (60 / V_MUL) < (30 / V_MUL))
    {
        // Display the press start text at a blink rate of 2hz

        // This used to be SpriteEngine_DrawUISprite_Large, but since it's only really used here
        // it's done this way to speed things along.
        struct spr_queue_entry s;
        s.signsize = 0x80; // Large sprite (32x32), positive X position
        
        int16_t y = 160;
        uint16_t base_tileattrib = 0x3000;

        int16_t x = 64;
        uint16_t tileattrib = base_tileattrib;
        s.y = y;

        // Draw a 128x32 sprite for the PRESS START text
        for (int px = 0; px < 4; px++)
        {
            s.x = x;
            s.tileattrib = tileattrib;
            SpriteEngine_DrawSprite(&s);
            x += 32;
            tileattrib += 4;
        }
    }

    SpriteEngine_ProcessSpriteLists();

    // Read for any button press (even though it says PRESS START, we accept any button)
    // TODO: make this go into a selection for new game/continue
    if (System_CheckKeyAny())
    {
        SoundInterface_StopMusic();
        SoundInterface_PlaySfx(SFX_ATK_PUNCH, 0); // Perhaps a better sound effect for this would be a "confirm" sound, but for now this is fine.

        shadow_brightness_change = 0;

        system_use_alternate_nmi = false;

        while (shadow_brightness > 0)
        {
            while ((REG_HVBJOY & VBL_READY) != VBL_READY)
            {
                ;
            }

            REG_INIDISP = shadow_brightness >> 8;

            while ((REG_HVBJOY & VBL_READY) == VBL_READY)
            {
                ;
            }

            shadow_brightness -= (64 * V_MUL);
        }
        shadow_brightness = 0;

        SpriteEngine_ResetSpriteLists();
        DmaSystem_ResetQueue();

        // Start the game intro
        system_loop_func_ptr = Main_GetFunctionPointer(ROUTINE_CUTSCENE_INIT);
    
        system_target_routine = ROUTINE_CUTSCENE_INIT;

        cs_current = (struct cutscene_data *)&data_cs_intro;
        
        System_Init_TilemapSettings(system_target_routine);
        System_Init_DisplaySettings(system_target_routine);
    }

    return;
}

void Title_Init()
{
    System_DisableInterrupts();

    // Initialize the title screen
    REG_INIDISP = 0x8f; // Disable display

    uint16_t transfer_length_1 = LZ4_UnpackToWRAM((void *)&data_bg_title_back_lz4, (uint8_t *)LZ4_BUFFER_ADDR);
    LZ4_UnpackToWRAM((void *)&data_tilemap_title_back_lz4, (uint8_t *)(LZ4_BUFFER_ADDR+0xc800));

    // Copy the palette
    DmaSystem_CopyToWram((uint8_t *)&data_palette_title, (uint8_t *)((uint32_t)&shadow_cgram+32), 224);

    // Upload the title screen
    DmaSystem_CopyToVram((uint8_t *)(LZ4_BUFFER_ADDR), 0x0000, transfer_length_1);
    DmaSystem_CopyToVram((uint8_t *)(LZ4_BUFFER_ADDR+0xc800), 0x3800, 1792);

    DmaSystem_UploadCgram();

    // Upload and start Module1 music sequence (5 channels: Kick, Snare, Hihat, Bass, Lead)
    SoundInterface_ResetSongInstruments();
    SoundInterface_UploadInstrumentList((struct sample_list_entry_ins *)&data_snd_instruments_module1[0]);
    
    SoundInterface_UploadMusicSequence(data_seq_module1_t1, 0);
    SoundInterface_UploadMusicSequence(data_seq_module1_t2, 1);
    SoundInterface_UploadMusicSequence(data_seq_module1_t3, 2);
    SoundInterface_UploadMusicSequence(data_seq_module1_t4, 3);
    SoundInterface_UploadMusicSequence(data_seq_module1_t5, 4);
    SoundInterface_SetMusicSpeed(6);
    SoundInterface_SetMusicTempo(125);
    SoundInterface_PlayMusic();

    REG_INIDISP = 0x00; // Enable display

    System_EnableInterrupts();

    shadow_brightness = 0;

    system_loop_func_ptr = Main_GetFunctionPointer(ROUTINE_TITLE);
    system_target_routine = ROUTINE_TITLE;

    System_Init_TilemapSettings(system_target_routine);
    System_Init_DisplaySettings(system_target_routine);

    return;
}
