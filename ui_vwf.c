#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "vars.h"

#include "dma.h"
#include "ui_vwf.h"

/*
    Prints text to a VWF buffer in WRAM

    Returns DMA transfer length in bytes.
*/
uint16_t VwfEngine_PrintText(uint8_t * string, uint8_t * dest, uint8_t * tilemap_dest, int col_ext, int row_ext, int id_offset)
{
    uint8_t * string_ptr = string;

    int shift = 0;
    int col = col_ext; // Tilemap current X
    int row = row_ext; // Tilemap current Y
    int tile_id = 1 | 0x2000;

    uint8_t * write_ptr_start = dest;
    uint8_t * write_ptr = (uint8_t *)((uint32_t)dest+16); // Skip 1 tile
    // Copy an empty tile to tile 0
    DmaSystem_CopyToWram((uint32_t)&const_zero, (uint32_t)dest, 16);

    DmaSystem_CopyToWram_ShortPrep(((uint32_t)&data_ui_vwf) >> 16, ((uint32_t)dest >> 16));

    uint16_t * tilemap_ptr = (uint16_t *)tilemap_dest;

    // Point guaranteed empty tiles to tile 0
    for (int y = 0; y < row; y++)
    {
        for (int x = 0; x < 32; x++)
        {
            *tilemap_ptr = 0x0000;
            tilemap_ptr++;
        }
    }

    // Then stub out the first columns
    for (int x = 0; x < col; x++)
    {
        *tilemap_ptr = 0x0000;
        tilemap_ptr++;
    }

    bool text_rendered = false;

    while (*string_ptr != 0x00)
    {
        // Efficient implementation
        if (*string_ptr == '\n')
        {
            int i;
            if (shift == 0)
            {
                i = 0;
            }
            else
            {
                i = 1;
                *tilemap_ptr = tile_id;
                tilemap_ptr++;
            }

            int remaining = (32-col)+col_ext;
            for (; i < remaining; i++)
            {
                *tilemap_ptr = 0x0000;
                tilemap_ptr++;
            }

            row++;
            col = col_ext;
            shift = 0;
            string_ptr++;

            if (text_rendered)
            {
                tile_id++;
                write_ptr += 16;
            }
            
            continue;
        }
        else if (col >= 32)
        {
            tilemap_ptr += (32-col)+col_ext;

            row++;
            col = col_ext;
            shift = 0;

            tile_id++;
            write_ptr += 16;
        }
        
        if (row >= 28)
        {
            break;
        }

        text_rendered = true;
        
        if (row >= 28)
        {
            break;
        }

        int glyph_sel = (*string_ptr)-0x20;

        int width = const_ui_vwf_offsets[glyph_sel];

        uint8_t * glyph_ptr = (uint8_t *)((uint32_t)&data_ui_vwf + (glyph_sel << 4));

        if (shift == 0)
        {
            // Copy the tile as is
            DmaSystem_CopyToWram_ShortRun((uint16_t)((uint32_t)glyph_ptr), (uint16_t)((uint32_t)write_ptr), 16);
        }
        else
        {
            uint8_t * write_ptr_saved = write_ptr; // Save the write pointer

            uint8_t shifted_glyph[32]; // 2 tiles
            uint16_t bitplane_mul = 1 << (8 - shift);
            uint16_t bitplane_row;

            // Bit shifting is needed
            for (int i = 0; i < 16; i++)
            {
                bitplane_row = *glyph_ptr;
                
                bitplane_row = (bitplane_row * bitplane_mul);
                shifted_glyph[i] = bitplane_row >> 8;
                shifted_glyph[16+i] = bitplane_row;

                *write_ptr |= shifted_glyph[i];
                *(write_ptr+16) = shifted_glyph[16+i];
                write_ptr++;

                glyph_ptr++;
            }

            write_ptr = write_ptr_saved; // Restore the write pointer
        }

        shift += width;

        *tilemap_ptr = tile_id;
        *(tilemap_ptr+1) = tile_id+1;

        if (shift >= 8)
        {
            shift -= 8;
            write_ptr += 16;
            tile_id++;
            tilemap_ptr++;
            col++;
        }

        string_ptr++;
    }

    // Write the remaining tilemap entries.
    tilemap_ptr++;

    for (int i = (row << 5) + col + 1; i < 896; i++)
    {
        *tilemap_ptr = 0x0000;
        tilemap_ptr++;
    }

    return (uint16_t)((uint32_t)(write_ptr-write_ptr_start)+16);
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
