#include <stdint.h>

#include "vars.h"

#include "ani_bg.h"

/*
    Updates water animation tiles with 512 byte chunks
*/
void ani_bg_update_water_anim()
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
    Updates bg animation tiles in the entire 128x32 section (2KB)
*/
void ani_bg_update_bg_anim()
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