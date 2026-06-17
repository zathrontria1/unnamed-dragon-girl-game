#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "ani_fixedspr.h"
#include "dma.h"

bool ani_coin_flip; // Used by the coin function to enable H flip.

/*
    Process DMA tile animations for sprites in the fixed area.
*/
void AniSystem_Spr_UpdateFixedTiles()
{
    if ((((uint16_t)system_frames_elapsed) & ANI_INTERVAL_8) == ANI_INTERVAL_8)
    {
        ani_bg_frame_coin++;
    }

    uint16_t temp_frame;

    if (ani_bg_frame_coin >= 11)
    {
        ani_bg_frame_coin = 0;
        temp_frame = 0;
    }
    else if (ani_bg_frame_coin >= 6)
    {
        temp_frame = 11 - (ani_bg_frame_coin + 1);
        ani_coin_flip = true;
    }
    else
    {
        temp_frame = ani_bg_frame_coin;
        ani_coin_flip = false;
    }

    // Calculate the new address
    uint8_t * temp_src = ((uint8_t *)&data_spr_drop_coin) + (temp_frame << 6); // 64 bytes
    uint16_t temp_dest = 0x62a0;

    if (temp_src != ani_bg_addr_coin)
    {
        if (DmaSystem_AddItemToQueue(temp_src, temp_dest, 128, VRAM_INCHIGH, 1) != 1)
        {
            ani_bg_addr_coin = temp_src;
        }
    }

    return;
}
