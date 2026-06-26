#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <snes/console.h>

#include "vars.h"

#include "spr.h"
#include "spr_metaspr.h"

#include "ani.h"

#include "obj.h"
#include "dma.h"

#include "routines_boss.h"

#include "math_int.h"

#include "ui.h"

#include "snd.h"
#include "consts_snd.h"

#include "hittest.h"

#include "gfx.h"

#include "movement.h"

#include "main.h"

#include "system.h"

uint16_t obj_boss_state; // Boss state machine variable

void Routines_Boss_Test(struct game_object * o)
{
    if (!system_game_paused)
    {
        struct game_object * p = obj_player_pointer;

        if ((o->state != STATE_DIE) && (o->state != STATE_SPAWNING))
        {
            // Place boss logic here
            event_in_combat = 1;
        }
        else if (o->state == STATE_DIE)
        {
            // Place events to take place when the boss dies here
            ;
        }
        else // spawning
        {
            // Delay when a boss is spawning
            event_in_combat = 1;
            
            o->struct_data.npc_data.status_time--;
            if (o->struct_data.npc_data.status_time == 0)
            {
                o->state = STATE_IDLE;
            }
        }
    }

    if ((system_frames_elapsed & 0x03) == 0x00)
    {
        o->struct_data.npc_data.ani.frame = ((o->struct_data.npc_data.ani.frame + 1) & 0x07); // 8 frame animation
    }

    Routines_Boss_Test_ReconstructFrame(o);
    Routines_Boss_Test_DmaFrame(o);

    SpriteEngine_AddMetaSprite(o, (const struct spr_metaspr_definition *)&data_metaspr_boss_generic_64x96);

    if (system_frames_elapsed & 0x01)
    {
        SpriteEngine_AddMetaSprite_Back(o, (const struct spr_metaspr_definition *)&data_metaspr_shadow_64x16);
    }

    return;
}

/*
    Reconstruct boss frame based on the lookup table

    They're basically 16x16 slices made from 16x8 slices
*/
void Routines_Boss_Test_ReconstructFrame(struct game_object * o)
{
    uint16_t frame_offset = o->struct_data.npc_data.ani.frame;

    Routines_Boss_Test_GetCompressedFrame((const uint8_t *)&data_spr_boss_placeholder_dd, (const uint16_t *)&data_spr_boss_placeholder_lut, (uint8_t *)(0x007fe000), frame_offset);

    /*for (int y = 0; y < 6; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            // a tile is 32 bytes.
            // A VRAM tile row is 512 bytes.
            // y = 96 pixels in height = 6 * 16px = 12 * 8px
            // x = 64 pixels in width = 4 * 16px = 8 * 8px
            // frame offsets: 0, 4, 24, 28
            // place lower half in right side to keep things linear for DMA

            uint16_t wram_offset;
            if (y >= 3)
            {
                wram_offset = 256 + (x << 6) + ((y - 3) << 10);
            }
            else
            {
                wram_offset = (x << 6) + (y << 10);
            }
            uint16_t frame_offset = ((o->struct_data.npc_data.ani.frame & 0x01) << 2) + ((o->struct_data.npc_data.ani.frame & 0x06) * 24);
            AniSystem_GetCompressedFrame((const uint8_t *)&data_spr_boss_placeholder_dd, (const uint16_t *)&data_spr_boss_placeholder_lut, (uint8_t *)(0x007fe000+wram_offset), (y << 3) + x + frame_offset);
        }
    }*/

    return;
}

void Routines_Boss_Test_DmaFrame(struct game_object * o)
{
    // a tile is 32 bytes.
    // A VRAM tile row is 512 bytes.
    // y = 96 pixels in height = 6 * 16px = 12 * 8px
    // x = 64 pixels in width = 4 * 16px = 8 * 8px

    DmaSystem_AddItemToQueue((uint8_t *)(0x007fe000), 0x7000, 3072, VRAM_INCHIGH, 0);
    /*for (int y = 0; y < 12; y++)
    {
        uint16_t wram_offset = y << 9;
        DmaSystem_AddItemToQueue((uint8_t *)(0x007fe000+wram_offset), 0x7000+(wram_offset >> 1), 256, VRAM_INCHIGH, 0);
    }*/

    return;
}

// Optimize this so that it builds the entire thing at once
uint8_t * Routines_Boss_Test_GetCompressedFrame(const uint8_t * data, const uint16_t * lookup, uint8_t * buffer, uint16_t frame)
{
    uint16_t lookup_entry_offset = frame * 48; // Each frame is 96 bytes. Use halved values

    uint8_t * ptr_return_val = (uint8_t *)(lookup + lookup_entry_offset);

    uint16_t * data_offset = (uint16_t *)ptr_return_val;

    //uint8_t * ptr_read = (uint8_t *)(data + *data_offset++);

    uint16_t ptr_array[48];

    for (int i = 0; i < 48; i++)
    {
        ptr_array[i] = (uint16_t)((uint32_t)(data + *data_offset++) & 0xffff);
    }

    // Use the DMA unit to speed things up. Channel 7 is reserved for active display DMA.
    // Align read
    REG_DMAP7 = 0x00; // byte reg write
    REG_BBAD7 = 0x80; // WMDATA

    REG_WMADDLM = (uint16_t)((uint32_t)buffer);
    REG_WMADDH = (uint8_t)((uint32_t)buffer >> 16);

    REG_A1T7LH = ptr_array[0];
    REG_A1B7 = (uint8_t)((uint32_t)data >> 16);

    REG_DAS7LH = 64;

    System_AlignToHblank();

    REG_MDMAEN = 0x80;

    int x = 1;
    int y = 0;

    for (int i = 1; i < 48; i++)
    {
        REG_A1T7LH = ptr_array[i];
        REG_DAS7LH = 64;
        REG_MDMAEN = 0x80;

        x++;

        if (x > 3)
        {
            x = 0;
            y++;

            if (y >= 6)
            {
                REG_WMADDLM = (uint16_t)((uint32_t)buffer + 256 + ((y - 6) << 9));
            }   
            else
            {
                REG_WMADDLM = (uint16_t)((uint32_t)buffer + (y << 9));
            }

            System_AlignToHblank();
        }
    }

    return ptr_return_val;
}
