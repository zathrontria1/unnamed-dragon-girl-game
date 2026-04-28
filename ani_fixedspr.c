#include <snes/console.h>
#include <stdint.h>

#include "vars.h"

#include "ani_fixedspr.h"
#include "dma.h"

void ani_fixedspr_process()
{
    if ((((uint16_t)system_frames_elapsed) & ANI_INTERVAL_8) == ANI_INTERVAL_8)
    {
        ani_bg_frame_coin++;
    }

    uint16_t temp_frame;

    if (ani_bg_frame_coin >= 13)
    {
        ani_bg_frame_coin = 0;
        temp_frame = 0;
    }
    else if (ani_bg_frame_coin >= 7)
    {
        temp_frame = 13 - ani_bg_frame_coin;
    }
    else
    {
        temp_frame = ani_bg_frame_coin;
    }

    // Calculate the new address
    uint8_t * temp_src = ((uint8_t *)&data_sprite_drop_coin) + (temp_frame << 6); // 64 bytes
    uint16_t temp_dest = 0x62a0;

    if (temp_src != ani_bg_addr_coin)
    {
        if (dma_queue_add(temp_src, temp_dest, 128, VRAM_INCHIGH, 1) != 1)
        {
            ani_bg_addr_coin = temp_src;
        }
    }
    

    return;
}
