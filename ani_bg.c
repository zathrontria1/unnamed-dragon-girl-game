#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "ani_bg.h"

/*
    Updates strip animation tiles with 512 byte chunks

    These animations update one row of tiles at a time (512 bytes = 16 4bpp tiles = 1 row of tiles in VRAM)
*/
void AniSystem_BgTile_UpdateStrip()
{
    if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_2) == ANI_INTERVAL_2)
    {
        ani_bg_row_water++;
        if (ani_bg_row_water > 3)
        {
            ani_bg_row_water = 0;
            ani_bg_frame_water++;

            if (ani_bg_frame_water > 7)
            {
                ani_bg_frame_water = 0;
            }
        }
    }

    // Calculate the new address
    uint8_t * temp_src = ((uint8_t *)&data_bg_dungeon_anim_water) + (ani_bg_frame_water << 11) + (ani_bg_row_water << 9);
    uint16_t temp_dest = 0x2000 + (ani_bg_row_water << 8);

    if (temp_src != ani_bg_addr_water)
    {
        ani_bg_addr_water = temp_src;
        ani_bg_dest_water = temp_dest;
        ani_bg_water_dma_ready = 1;
    }

    return;
}

/*
    Update frame animation tiles with 2048 byte chunks

    These animations update four row of tiles at a time (2048 bytes = 64 4bpp tiles = 4 row of tiles in VRAM)
    Equivalent to updating a 128x32 section.
*/
void AniSystem_BgTile_UpdateFrame()
{
    if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_4) == ANI_INTERVAL_4)
    {
        ani_bg_frame_tallbg++;

        if (ani_bg_frame_tallbg > 3)
        {
            ani_bg_frame_tallbg = 0;
        }
    }

    // Calculate the new address
    uint8_t * temp_src = ((uint8_t *)&data_bg_dungeon_anim_torch) + (ani_bg_frame_tallbg << 11);
    uint16_t temp_dest = 0x2400;

    if (temp_src != ani_bg_addr_tallbg)
    {
        ani_bg_addr_tallbg = temp_src;
        ani_bg_dest_tallbg = temp_dest;
        ani_bg_tallbg_dma_ready = 1;
    }

    return;
}