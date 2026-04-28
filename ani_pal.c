#include <snes/console.h>
#include <stdint.h>

#include "vars.h"

#include "ani_pal.h"

/*
    Sets the palette entry based on current frame
*/
void ani_pal_process()
{
    if ((((uint16_t)system_frames_elapsed) & ANI_INTERVAL_8) == ANI_INTERVAL_8)
    {
        pal_ani_sel++;
    }

    uint16_t temp_sel;

    if (pal_ani_sel >= 14)
    {
        pal_ani_sel = 0;
        temp_sel = 0;
    }
    else if (pal_ani_sel >= 8)
    {
        temp_sel = 14 - pal_ani_sel;
    }
    else
    {
        temp_sel = pal_ani_sel;
    }

    shadow_cgram.entry[0x5e] = pal_ani_entries[temp_sel][0];
    shadow_cgram.entry[0x5f] = pal_ani_entries[temp_sel][1];

    return;
}

/*
    Pre-calculates palette entries
*/
void ani_pal_precalc_entries(void)
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            uint16_t temp_colour;

            int16_t temp_r;
            int16_t temp_g;
            int16_t temp_b;

            temp_r = ((shadow_cgram.entry[(0x5e)+j] & 0x001f) * (31 - i)) / 31;
            temp_g = (((shadow_cgram.entry[(0x5e)+j] & 0x03e0) >> 5) * (31 - i)) / 31;
            temp_b = (((shadow_cgram.entry[(0x5e)+j] & 0x7c00) >> 10) * (31 - i)) / 31;
            
            if (temp_r < 0)
            {
                temp_r = 0;
            }
            else if (temp_r > 31)
            {
                temp_r = 31;
            }

            if (temp_g < 0)
            {
                temp_g = 0;
            }
            else if (temp_g > 31)
            {
                temp_g = 31;
            }

            if (temp_b < 0)
            {
                temp_b = 0;
            }
            else if (temp_b > 31)
            {
                temp_b = 31;
            }

            pal_ani_entries[i][j] = RGB5(temp_r, temp_g, temp_b);
        }
    }

    return;
}
