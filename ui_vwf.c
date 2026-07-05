#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "vars.h"

#include "consts_snd.h"

#include "dma.h"
#include "system.h"
#include "ui_vwf.h"

#include "snd.h"

// VWF engine state, needed for gradual printing
uint16_t vwf_shift;

uint16_t vwf_col_start;
uint16_t vwf_col;

uint16_t vwf_row_start;
uint16_t vwf_row;

uint16_t vwf_tile_id;
uint16_t vwf_tile_id_empty;
uint8_t * vwf_string_ptr;
uint16_t * vwf_tilemap_ptr;
uint16_t * vwf_tilemap_ptr_start;
uint16_t vwf_tilemap_len;

uint8_t * vwf_tiledata_ptr;
uint8_t * vwf_tiledata_ptr_start;
bool vwf_text_rendered;

uint16_t vwf_wram_offset;
uint16_t vwf_vram_offset;

// Return value for length of next VWF DMA run
uint16_t vwf_tiledata_run;
uint16_t vwf_tiledata_advance; // And how much to advance the tile pointer, which is separate/cannot be inferred
uint16_t vwf_tiledata_advance_vram;

uint16_t vwf_run_width;
uint16_t vwf_advance_width;

// Is VWF engine mid-run?
// This should be checked before starting a single-character print command.
// Starting a new command will reset state.
bool vwf_print_ongoing;
// Has VWF engine finished printing? If this is set, don't call printing functions
bool vwf_print_finished;

/*
    Prints text to a VWF buffer in WRAM

    Returns DMA transfer length in bytes.

    Tilemap_dest should be byte-indexed! Word indexing will be handled automatically
*/
uint16_t VwfEngine_PrintText(uint8_t * string, uint8_t * dest, uint8_t * tilemap_dest, int col_ext, int row_ext, int id_offset)
{
    VwfEngine_PrintText_Gradual_Setup(string, dest, tilemap_dest, col_ext, row_ext, id_offset, 896);
    VwfEngine_PrintText_Gradual(32767);
    return vwf_tiledata_run+16;
}

/*
    Begin a new text print

    tilemap_length is in amount of entries (words)
*/
void VwfEngine_PrintText_Gradual_Setup(uint8_t * string, uint8_t * dest, uint8_t * tilemap_dest, int col_ext, int row_ext, int id_offset, int tilemap_len)
{
    vwf_shift = 0;

    vwf_col_start = col_ext;
    vwf_col = col_ext;

    vwf_row_start = row_ext;
    vwf_row = row_ext;

    vwf_tile_id = (1 + id_offset) | 0x2000;
    vwf_tile_id_empty = id_offset;
    vwf_string_ptr = string;
    vwf_tilemap_ptr = (uint16_t *)tilemap_dest;
    vwf_tilemap_ptr_start = (uint16_t *)tilemap_dest;
    vwf_tilemap_len = tilemap_len;

    vwf_tiledata_ptr = dest+16;
    vwf_tiledata_ptr_start = dest;
    vwf_text_rendered = false;

    vwf_wram_offset = (uint16_t)((uint32_t)vwf_tiledata_ptr_start);
    vwf_vram_offset = 0x0008;

    vwf_tiledata_run = 16;
    vwf_tiledata_advance = 0;
    vwf_tiledata_advance_vram = 0;

    vwf_print_ongoing = true;
    vwf_print_finished = false;

    // Copy an empty tile to tile 0
    System_CopyBlock((uint8_t *)&const_zero, dest, 16);

    VwfEngine_PrintText_ResetTilemap(vwf_tilemap_ptr, vwf_tilemap_len);

    SoundInterface_PlayClip(STREAM_TYPEWRITER);

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

    vwf_run_width = 0;
    vwf_advance_width = 0;

    bool shift_overflow = false;

    if (!vwf_print_ongoing || (len != 32767)) // 32767 is instant
    {
        SoundInterface_PlayClip(STREAM_TYPEWRITER);
    }
    
    vwf_print_ongoing = true;

    for (int c = 0; c < len; c++)
    {
        if (*vwf_string_ptr != 0x00)
        {
            // Efficient implementation
            if (*vwf_string_ptr == '\n')
            {
                VwfEngine_PrintText_Internal_AlignPointers();

                vwf_row++;
                
                continue;
            }
            else if (*vwf_string_ptr == '\r')
            {
                VwfEngine_PrintText_Internal_AlignPointers();

                vwf_row = vwf_row_start;

                vwf_print_ongoing = false;

                break;
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

            int glyph_sel = (*vwf_string_ptr);

            int width = const_ui_vwf_offsets[glyph_sel];

            uint8_t * glyph_ptr = (uint8_t *)&data_ui_vwf + (glyph_sel << 4);

            if (shift_overflow)
            {
                vwf_run_width++;
                shift_overflow = false;
            }

            if (vwf_shift == 0)
            {
                // Copy the tile as is
                System_CopyBlock(glyph_ptr, vwf_tiledata_ptr, 16);
                if (vwf_run_width == 0)
                {
                    vwf_run_width = 1;
                }
            }
            else
            {
                uint16_t bitplane_mul = 1 << (8 - vwf_shift);

                VwfEngine_PrintText_Render(glyph_ptr, vwf_tiledata_ptr, bitplane_mul);

                if (vwf_run_width < 2)
                {
                    vwf_run_width = 2;
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
                vwf_advance_width++;
            }

            vwf_string_ptr++;
        }

        if (*vwf_string_ptr == 0x00)
        {
            vwf_print_finished = true;
            vwf_print_ongoing = false;
            break;
        }
    }

    if (shift_overflow) // One extra catch
    {
        vwf_run_width++;
        shift_overflow = false;
    }


    if (!vwf_print_ongoing) // Same rationale as the beginning one
    {
        SoundInterface_StopStream();
    }

    vwf_tiledata_run = vwf_run_width << 4;
    vwf_tiledata_advance = vwf_advance_width << 4;
    vwf_tiledata_advance_vram = vwf_advance_width << 3;

    return write_ptr_start;
}

/*
    Called when something happens that require pointer resets

    e.g. new lines or new pages
*/
void VwfEngine_PrintText_Internal_AlignPointers()
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
    
    vwf_col = vwf_col_start;
    vwf_shift = 0;
    vwf_string_ptr++;

    if (vwf_text_rendered)
    {
        vwf_run_width++;
        vwf_advance_width++;
        vwf_tile_id++;
        vwf_tiledata_ptr += 16;
    }

    return;
}

#if VBCC_ASM == 1
NO_INLINE void VwfEngine_PrintText_Render(__reg("r4/r5") uint8_t * glyph_ptr, __reg("r6/r7")uint8_t * write_ptr, __reg("a")uint16_t mul)
#else
void VwfEngine_PrintText_Render(uint8_t * glyph_ptr, uint8_t * write_ptr, uint16_t mul)
#endif
{
    #if VBCC_ASM == 1
        __asm(
        "\ta8\n"
        "\tx16\n"
        "\tsep #$20\n"
        "\tsta $4202\n" // Multiplication factor constant

        "\tlda r7\n"
        "\tsta r9\n"
        
        "\ta16\n"
        "\trep #$21\n"

        "\tlda r6\n"
        "\tadc #16\n"
        "\tsta r8\n"
        
        "\tldy #$0\n"
        
        "\ta8\n"
        "\tsep #$20\n"

        "\trept 16, I\n"
        "\ta8\n"

        "\tlda [r4],y\n"
        "\tsta $4203\n"
        
        "\ta16\n"
        "\trep #$20\n"

        "\tnop\n"

        "\tlda $4216\n"

        "\ta8\n"
        "\tsep #$20\n"
        "\tsta [r8],y\n"
        "\txba\n"
        "\tora [r6],y\n"
        "\tsta [r6],y\n"

        "\tiny\n"
        
        "\tendr\n"
        
        "\ta16\n"
        "\tx16\n"
        "\trep #$30\n"
        );
    #else
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
    #endif

    return;
}

/*
    Called for resetting pointers when rendering multi-page text.
*/
void VwfEngine_PrintText_StartNewPage()
{
    vwf_wram_offset = (uint16_t)((uint32_t)vwf_tiledata_ptr_start);
    vwf_vram_offset = 0x0008;

    vwf_tiledata_ptr = vwf_tiledata_ptr_start+16;
    vwf_tilemap_ptr = vwf_tilemap_ptr_start;

    VwfEngine_PrintText_ResetTilemap(vwf_tilemap_ptr, vwf_tilemap_len);

    vwf_tiledata_advance = 0;
    vwf_tiledata_advance_vram = 0;

    return;
}

void VwfEngine_PrintText_ResetTilemap(uint16_t * ptr, int len)
{
    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\tsta r0\n"
            "\tstx r1\n"

            "\tlda 4,s\n"
            "\tbeq .end\n"

            "\tdec\n"
            "\tasl\n"
            "\ttay\n"

            "\tlda _vwf_tile_id_empty\n"
            
            ".loop:\n"
            
            "\tsta [r0],y\n"
            "\tdey\n"
            "\tdey\n"
            "\tbpl .loop\n"

            ".end:\n"
        );
    #else
        // Immediately write empty tiles to the tilemap 
        for (int i = 0; i < len; i++)
        {
            *ptr = vwf_tile_id_empty;
            ptr++;
        }
    #endif

    vwf_tile_id = (1 + vwf_tile_id_empty) | 0x2000;

    vwf_tilemap_ptr += (vwf_row_start << 5) + vwf_col_start;

    return;
}

const uint8_t const_ui_vwf_offsets[] = 
{
    8, 8, 8, 4, 6, 6, 6, 6, 8, 8, 8, 7, 7, 8, 6, 6,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,

    4, 2, 4, 8, 6, 8, 6, 2, 4, 4, 6, 6, 2, 4, 2, 5, 
    5, 4, 5, 5, 6, 5, 5, 6, 5, 5, 2, 3, 4, 4, 4, 5, 

    8, 6, 5, 5, 6, 6, 5, 7, 7, 4, 6, 6, 5, 8, 7, 7, 
    5, 8, 6, 6, 6, 6, 6, 8, 7, 6, 7, 3, 5, 3, 5, 7, 

    3, 5, 5, 5, 5, 5, 5, 5, 5, 2, 4, 5, 2, 6, 5, 5, 
    5, 5, 4, 4, 4, 5, 4, 6, 6, 6, 5, 5, 2, 5, 5, 5, 
};
