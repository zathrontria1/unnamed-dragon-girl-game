#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "vars.h"

#include "dma.h"
#include "system.h"
#include "ui_vwf.h"

// VWF engine state, needed for gradual printing
uint16_t vwf_shift;
uint16_t vwf_col_start;
uint16_t vwf_col;
uint16_t vwf_row;
uint16_t vwf_tile_id;
uint8_t * vwf_string_ptr;
uint16_t * vwf_tilemap_ptr;
uint8_t * vwf_tiledata_ptr;
bool vwf_text_rendered;

// Return value for length of next VWF DMA run
uint16_t vwf_tiledata_run;
uint16_t vwf_tiledata_advance; // And how much to advance the tile pointer, which is separate/cannot be inferred
uint16_t vwf_tiledata_advance_vram;

// Is VWF engine mid-run?
// This should be checked before starting a single-character print command.
// Starting a new command will reset state.
bool vwf_print_ongoing;
// Has VWF engine finished printing? If this is set, don't call printing functions
bool vwf_print_finished;

/*
    Prints text to a VWF buffer in WRAM

    Returns DMA transfer length in bytes.
*/
uint16_t VwfEngine_PrintText(uint8_t * string, uint8_t * dest, uint16_t * tilemap_dest, int col_ext, int row_ext, int id_offset)
{
    uint8_t * string_ptr = string;

    int shift = 0;
    int col = col_ext; // Tilemap current X
    int row = row_ext; // Tilemap current Y
    int tile_id = (1 + id_offset) | 0x2000;

    uint8_t * write_ptr_start = dest;
    uint8_t * write_ptr = (uint8_t *)((uint32_t)dest+16); // Skip 1 tile
    // Copy an empty tile to tile 0
    DmaSystem_CopyToWram((uint32_t)&const_zero, (uint32_t)dest, 16);

    DmaSystem_CopyToWram_ShortPrep(((uint32_t)&data_ui_vwf) >> 16, ((uint32_t)dest >> 16));

    uint16_t * tilemap_ptr = tilemap_dest;

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

/*
    Begin a new text print
*/
void VwfEngine_PrintText_Gradual_Setup(uint8_t * string, uint8_t * dest, uint16_t * tilemap_dest, int col_ext, int row_ext, int id_offset)
{
    vwf_shift = 0;

    vwf_col_start = col_ext;
    vwf_col = col_ext;
    vwf_row = row_ext;
    vwf_tile_id = (1 + id_offset) | 0x2000;
    vwf_string_ptr = string;
    vwf_tilemap_ptr = tilemap_dest;
    vwf_tiledata_ptr = dest+16;
    vwf_text_rendered = false;

    vwf_tiledata_run = 16;
    vwf_tiledata_advance = 0;
    vwf_tiledata_advance_vram = 0;

    vwf_print_ongoing = true;
    vwf_print_finished = false;

    // Copy an empty tile to tile 0
    System_CopyBlock((uint8_t *)&const_zero, dest, 16);

    // Immediately write empty tiles to the tilemap 
    for (int i = 0; i < 896; i++)
    {
        *tilemap_dest = id_offset;
        tilemap_dest++;
    }

    vwf_tilemap_ptr += (vwf_row << 5) + vwf_col;

    return;
}

/*
    Prints exactly length amount of characters to VWF buffer.

    Returns the tile data address where the start of updated character information resides.

    Length is stored in vwf_tiledata_run as a global variable.

    It is assumed that the caller will copy all relevant sections manually and length is up to the caller
*/
uint8_t * VwfEngine_PrintText_Gradual(int len)
{
    uint8_t * write_ptr_start = vwf_tiledata_ptr;

    int run_width = 0;
    int advance_width = 0;

    bool shift_overflow = false;

    for (int c = 0; c < len; c++)
    {
        if (*vwf_string_ptr != 0x00)
        {
            // Efficient implementation
            if (*vwf_string_ptr == '\n')
            {
                int i;
                if (vwf_shift == 0)
                {
                    i = 0;
                }
                else
                {
                    i = 1;
                    *vwf_tilemap_ptr = vwf_tile_id;
                    vwf_tilemap_ptr++;
                }

                int remaining = (32-vwf_col)+vwf_col_start;
                for (; i < remaining; i++)
                {
                    *vwf_tilemap_ptr = 0x0000;
                    vwf_tilemap_ptr++;
                }
                
                vwf_row++;
                vwf_col = vwf_col_start;
                vwf_shift = 0;
                vwf_string_ptr++;

                if (vwf_text_rendered)
                {
                    run_width++;
                    advance_width++;
                    vwf_tile_id++;
                    vwf_tiledata_ptr += 16;
                }
                
                continue;
            }
            else if (vwf_col >= 32)
            {
                vwf_tilemap_ptr += (32-vwf_col)+vwf_col_start;

                vwf_row++;
                vwf_col = vwf_col_start;
                vwf_shift = 0;

                vwf_tile_id++;
                vwf_tiledata_ptr += 16;
            }
            
            if (vwf_row >= 28)
            {
                break;
            }

            vwf_text_rendered = true;

            int glyph_sel = (*vwf_string_ptr)-0x20;

            int width = const_ui_vwf_offsets[glyph_sel];

            uint8_t * glyph_ptr = (uint8_t *)((uint32_t)&data_ui_vwf + (glyph_sel << 4));

            if (shift_overflow)
            {
                run_width++;
                shift_overflow = false;
            }

            if (vwf_shift == 0)
            {
                // Copy the tile as is
                System_CopyBlock(glyph_ptr, vwf_tiledata_ptr, 16);
                if (run_width == 0)
                {
                    run_width = 1;
                }
            }
            else
            {
                uint8_t * write_ptr_saved = vwf_tiledata_ptr; // Save the write pointer

                uint8_t shifted_glyph[32]; // 2 tiles
                uint16_t bitplane_mul = 1 << (8 - vwf_shift);
                uint16_t bitplane_row;

                // Bit shifting is needed
                for (int i = 0; i < 16; i++)
                {
                    bitplane_row = *glyph_ptr;
                    
                    bitplane_row = (bitplane_row * bitplane_mul);
                    shifted_glyph[i] = bitplane_row >> 8;
                    shifted_glyph[16+i] = bitplane_row;

                    *vwf_tiledata_ptr |= shifted_glyph[i];
                    *(vwf_tiledata_ptr+16) = shifted_glyph[16+i];
                    vwf_tiledata_ptr++;

                    glyph_ptr++;
                }

                vwf_tiledata_ptr = write_ptr_saved; // Restore the write pointer

                if (run_width < 2)
                {
                    run_width = 2;
                }
            }

            *vwf_tilemap_ptr = vwf_tile_id;
            if (vwf_shift != 0)
            {
                *(vwf_tilemap_ptr+1) = vwf_tile_id+1;
            }

            vwf_shift += width;

            if (vwf_shift >= 8)
            {
                vwf_shift -= 8;
                vwf_tiledata_ptr += 16;
                vwf_tile_id++;
                vwf_tilemap_ptr++;
                vwf_col++;

                shift_overflow = true;
                advance_width++;
            }

            vwf_string_ptr++;
        }

        if (*vwf_string_ptr == 0x00)
        {
            vwf_print_finished = true;
            break;
        }
    }

    vwf_tiledata_run = run_width << 4;
    vwf_tiledata_advance = advance_width << 4;
    vwf_tiledata_advance_vram = advance_width << 3;

    return write_ptr_start;
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
