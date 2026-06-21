#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "vars.h"

#include "dma.h"
#include "ui_vwf.h"

/*
    Prints text to a VWF buffer in WRAM
*/
void VwfEngine_PrintText(uint8_t * string, uint8_t * dest, uint8_t * tilemap_dest)
{
    DmaSystem_ClearWram((uint32_t)dest, 14336);

    uint8_t * string_ptr = string;

    int shift = 0;
    int col = 2;
    int row = 1;

    DmaSystem_CopyToWram_ShortPrep(((uint32_t)&data_ui_vwf) >> 16, ((uint32_t)dest >> 16));

    while (*string_ptr != 0x00)
    {
        if (*string_ptr == '\n')
        {
            row++;
            col = 2;
            shift = 0;
            string_ptr++;
            continue;
        }
        else if (col >= 32)
        {
            row++;
            col = 2;
            shift = 0;
        }
        
        if (row >= 28)
        {
            break;
        }

        int glyph_sel = (*string_ptr)-0x20;

        int width = const_ui_vwf_offsets[glyph_sel];

        uint8_t * glyph_ptr = (uint8_t *)((uint32_t)&data_ui_vwf + (glyph_sel << 4));
        uint16_t * write_ptr = (uint16_t *)((uint32_t)dest + (col << 4) + (row << 9));

        if (shift == 0)
        {
            // Copy the tile as is
            DmaSystem_CopyToWram_ShortRun((uint16_t)((uint32_t)glyph_ptr), (uint16_t)((uint32_t)write_ptr), 16);
        }
        else
        {
            uint8_t shifted_glyph[32]; // 2 tiles

            // Bit shifting is needed
            for (int i = 0; i < 16; i++)
            {
                uint8_t bitplane_row = *glyph_ptr;

                // Make sure to clean up the second half
                shifted_glyph[16+i] = 0x00;

                for (int j = 0; j < shift; j++)
                {
                    if (bitplane_row & 0x01)
                    {
                        bitplane_row >>= 1;
                        // Set bit
                        shifted_glyph[i] = bitplane_row;
                        shifted_glyph[16+i] = (shifted_glyph[16+i] >> 1) | 0x80;
                    }
                    else
                    {
                        bitplane_row >>= 1;
                        // Clear bit
                        shifted_glyph[i] = bitplane_row;
                        shifted_glyph[16+i] = (shifted_glyph[16+i] >> 1);
                    }
                }

                glyph_ptr++;
            }

            // Then OR two tiles
            uint16_t * shifted_ptr = (uint16_t *)&shifted_glyph[0];

            for (int i = 0; i < 16; i++)
            {
                *write_ptr |= *shifted_ptr++;
                write_ptr++;
            }
        }

        shift += width;

        if (shift >= 8)
        {
            shift -= 8;
            col++;
        }

        string_ptr++;
    }

    // Generate the tilemap
    // Right now it's hardcoded to 0x7000 from start
    // As it's used for the error message
    uint16_t * tilemap_ptr = (uint16_t *)tilemap_dest;

    for (int i = 0; i < 896; i++)
    {
        *tilemap_ptr = i;

        tilemap_ptr++;
    }

    return;
}

const uint16_t const_ui_vwf_offsets[] = 
{
    5, 2, 4, 8, 6, 8, 6, 2, 4, 4, 6, 6, 2, 4, 2, 5, 
    5, 4, 5, 5, 6, 5, 5, 6, 5, 5, 2, 3, 4, 4, 4, 5, 
    8, 6, 5, 5, 6, 6, 5, 7, 7, 4, 6, 6, 5, 8, 7, 7, 
    5, 8, 6, 6, 6, 6, 6, 8, 7, 6, 7, 3, 5, 3, 5, 7, 
    3, 5, 5, 5, 5, 5, 5, 5, 5, 2, 4, 5, 2, 6, 5, 5, 
    5, 5, 4, 4, 4, 5, 4, 6, 6, 6, 5, 5, 2, 5, 5, 5, 
};
