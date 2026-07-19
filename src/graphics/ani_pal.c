#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "ani_pal.h"
#include "system.h"

uint16_t pal_ani_entries[8][2]; // Just enough for the magic circle
uint16_t pal_ani_sel;

uint16_t pal_cycle_entries[16][16]; // Covers 16 subpalettes as a back buffer for copying later.

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
    This one copies to a cycle entry subpalette instead

    This will avoid clobbering the main shadow in case some entries need to be used
    without being touched
*/
void AniSystem_Pal_LoadCycleSubpalette(uint8_t * ptr, uint16_t subpal)
{
    uint8_t * dest_ptr = (uint8_t *)&pal_cycle_entries + (subpal << 5);
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
    to calculate them during live gameplay.

    This is called during setup, not in the main game loop
*/
void AniSystem_Pal_PrecalcPaletteChanges(void)
{
    // Copy the entries themselves into temporary variables
    uint16_t entry0 = shadow_cgram.entry[0x17];
    uint16_t entry1 = shadow_cgram.entry[0x18];

    // Extract the RGB components of each entry, and scale them down to 5-bit values
    uint16_t r0 = entry0 & 0x001f;
    uint16_t g0 = (entry0 >> 5) & 0x001f;
    uint16_t b0 = (entry0 >> 10) & 0x001f;

    uint16_t r1 = entry1 & 0x001f;
    uint16_t g1 = (entry1 >> 5) & 0x001f;
    uint16_t b1 = (entry1 >> 10) & 0x001f;

    // Write out every entry (with direct array accesses). Use the scale table to avoid complex math.
    pal_ani_entries[0][0] = RGB5(const_pal_scale_table[0][r0], const_pal_scale_table[0][g0], const_pal_scale_table[0][b0]);
    pal_ani_entries[0][1] = RGB5(const_pal_scale_table[0][r1], const_pal_scale_table[0][g1], const_pal_scale_table[0][b1]);

    pal_ani_entries[1][0] = RGB5(const_pal_scale_table[1][r0], const_pal_scale_table[1][g0], const_pal_scale_table[1][b0]);
    pal_ani_entries[1][1] = RGB5(const_pal_scale_table[1][r1], const_pal_scale_table[1][g1], const_pal_scale_table[1][b1]);

    pal_ani_entries[2][0] = RGB5(const_pal_scale_table[2][r0], const_pal_scale_table[2][g0], const_pal_scale_table[2][b0]);
    pal_ani_entries[2][1] = RGB5(const_pal_scale_table[2][r1], const_pal_scale_table[2][g1], const_pal_scale_table[2][b1]);

    pal_ani_entries[3][0] = RGB5(const_pal_scale_table[3][r0], const_pal_scale_table[3][g0], const_pal_scale_table[3][b0]);
    pal_ani_entries[3][1] = RGB5(const_pal_scale_table[3][r1], const_pal_scale_table[3][g1], const_pal_scale_table[3][b1]);

    pal_ani_entries[4][0] = RGB5(const_pal_scale_table[4][r0], const_pal_scale_table[4][g0], const_pal_scale_table[4][b0]);
    pal_ani_entries[4][1] = RGB5(const_pal_scale_table[4][r1], const_pal_scale_table[4][g1], const_pal_scale_table[4][b1]);

    pal_ani_entries[5][0] = RGB5(const_pal_scale_table[5][r0], const_pal_scale_table[5][g0], const_pal_scale_table[5][b0]);
    pal_ani_entries[5][1] = RGB5(const_pal_scale_table[5][r1], const_pal_scale_table[5][g1], const_pal_scale_table[5][b1]);

    pal_ani_entries[6][0] = RGB5(const_pal_scale_table[6][r0], const_pal_scale_table[6][g0], const_pal_scale_table[6][b0]);
    pal_ani_entries[6][1] = RGB5(const_pal_scale_table[6][r1], const_pal_scale_table[6][g1], const_pal_scale_table[6][b1]);

    pal_ani_entries[7][0] = RGB5(const_pal_scale_table[7][r0], const_pal_scale_table[7][g0], const_pal_scale_table[7][b0]);
    pal_ani_entries[7][1] = RGB5(const_pal_scale_table[7][r1], const_pal_scale_table[7][g1], const_pal_scale_table[7][b1]);

    return;
}

/*
    Cycles a back-buffered subpalette

    Functionally, basically shifts all palette entries 1 entry to the right
*/
void AniSystem_Pal_CycleSubpalette(uint16_t subpal)
{
    // Use pointer offset calculation to avoid 2D array overhead
    uint16_t * ptr = (uint16_t *)((uint8_t *)pal_cycle_entries + (subpal << 5));

    // Get the last entry of the affected subpalette
    uint16_t last = ptr[15];

    // Shift everything in that subpalette to the right (unrolled to eliminate loop & dynamic index overhead)
    ptr[15] = ptr[14];
    ptr[14] = ptr[13];
    ptr[13] = ptr[12];
    ptr[12] = ptr[11];
    ptr[11] = ptr[10];
    ptr[10] = ptr[9];
    ptr[9] = ptr[8];
    ptr[8] = ptr[7];
    ptr[7] = ptr[6];
    ptr[6] = ptr[5];
    ptr[5] = ptr[4];
    ptr[4] = ptr[3];
    ptr[3] = ptr[2];
    ptr[2] = ptr[1];
    ptr[1] = ptr[0];

    // Finally put the last entry in the first entry
    ptr[0] = last;

    return;
}

/*
    Finally, this copies from cycled to main shadow cgram, and a length can be specified.

    Offsets are in 16-bit wide entry terms

    start = first entry
    len = entry count
*/
void AniSystem_Pal_CopyCycledSubpaletteToMainSubpalette(uint16_t src_subpal, uint16_t dest_subpal, uint16_t start, uint16_t len)
{
    uint8_t * src_ptr = (uint8_t *)&pal_cycle_entries + (src_subpal << 5) + (start << 1);
    uint8_t * dest_ptr = (uint8_t *)&shadow_cgram + (dest_subpal << 5) + (start << 1);
    System_CopyBlock(src_ptr, dest_ptr, len << 1);
    
    return;
}

/*
    Partial table of pre-calculated scale values for palette animation
*/
NEAR const uint16_t const_pal_scale_table[8][32] = {
    // i = 0 (31/31)
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 },
    // i = 1 (30/31)
    { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 },
    // i = 2 (29/31)
    { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29 },
    // i = 3 (28/31)
    { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 },
    // i = 4 (27/31)
    { 0, 0, 1, 2, 3, 4, 5, 6, 6, 7, 8, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 20, 20, 21, 22, 23, 24, 25, 26, 27 },
    // i = 5 (26/31)
    { 0, 0, 1, 2, 3, 4, 5, 5, 6, 7, 8, 9, 10, 10, 11, 12, 13, 14, 15, 15, 16, 17, 18, 19, 20, 20, 21, 22, 23, 24, 25, 26 },
    // i = 6 (25/31)
    { 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8, 9, 10, 11, 12, 12, 13, 14, 15, 16, 16, 17, 18, 19, 20, 20, 21, 22, 23, 24, 25 },
    // i = 7 (24/31)
    { 0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 10, 10, 11, 12, 13, 13, 14, 15, 16, 17, 17, 18, 19, 20, 20, 21, 22, 23, 24 }
};
