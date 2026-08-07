#include <stdint.h>

#include "vars.h"

#include "ui_vwf.h"

#if VBCC_ASM == 0
void VwfEngine_PrintText_Render(uint8_t * glyph_ptr, uint8_t * write_ptr, uint16_t mul)
{
    uint16_t bitplane_row;

    // Bit shifting is needed
    for (int i = 0; i < 16; i++)
    {
        if (!(*glyph_ptr)) // Bitplane row is all 0s
        {
            *(write_ptr+16) = 0x00;
        }
        else
        {
            bitplane_row = (*glyph_ptr * mul);

            *write_ptr |= bitplane_row >> 8;
            *(write_ptr+16) = bitplane_row;
        }
        write_ptr++;
        glyph_ptr++;

    }

    return;
}

void VwfEngine_PrintText_ResetTilemap(uint16_t * ptr, int len)
{
    // Immediately write empty tiles to the tilemap 
    for (int i = 0; i < len; i++)
    {
        *ptr = vwf_tile_id_empty;
        ptr++;
    }

    vwf_tile_id = (1 + vwf_tile_id_empty) | 0x2000;

    vwf_tilemap_ptr += (vwf_row_start << 5) + vwf_col_start;

    return;
}
#endif
