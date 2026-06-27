#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "ani_pal.h"
#include "system.h"

NEAR uint16_t pal_ani_entries[8][2]; // Just enough for the magic circle
uint16_t pal_ani_sel;

/*
    Copy a subpalette (16 entries/32 bytes) to the shadow

    ptr = pointer to subpalette
    subpal = subpalette (0-15)
*/
void AniSystem_Pal_LoadSubpalette(uint8_t * ptr, uint16_t subpal)
{
    uint8_t * dest_ptr = (uint8_t *)&shadow_cgram + (subpal << 5);
    System_CopyBlock(ptr, dest_ptr, 32);

    return;
}

/*
    Sets the palette entry based on current frame

    Uses pre-calculated entries
*/
void AniSystem_Pal_UpdatePalettes()
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

    shadow_cgram.entry[0x17] = pal_ani_entries[temp_sel][0];
    shadow_cgram.entry[0x18] = pal_ani_entries[temp_sel][1];

    return;
}

/*
    Pre-calculates palette entries for animation use so we don't need
    to calculate them during live gameplay
*/
void AniSystem_Pal_PrecalcPaletteChanges(void)
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            uint16_t temp_colour;

            int16_t temp_r;
            int16_t temp_g;
            int16_t temp_b;

            temp_r = ((shadow_cgram.entry[(0x17)+j] & 0x001f) * (31 - i)) / 31;
            temp_g = (((shadow_cgram.entry[(0x17)+j] & 0x03e0) >> 5) * (31 - i)) / 31;
            temp_b = (((shadow_cgram.entry[(0x17)+j] & 0x7c00) >> 10) * (31 - i)) / 31;
            
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

/*
    Cycles a subpalette

    Functionally, basically shifts all palette entries 1 entry to the right
*/
void AniSystem_Pal_CycleSubpalette(uint16_t subpal)
{
    // Get the last entry of the affected subpalette
    uint16_t last = shadow_cgram.entry[(subpal << 4) + 15];

    // Then shift everything in that subpalette to the right.
    for (int i = 15; i > 0; i--)
    {
        shadow_cgram.entry[(subpal << 4) + i] = shadow_cgram.entry[(subpal << 4) + i - 1];
    }

    // Finally put the last entry in the first entry
    shadow_cgram.entry[subpal << 4] = last;

    return;
}