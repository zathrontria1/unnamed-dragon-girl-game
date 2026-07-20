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
bool vwf_text_prev_is_newline;

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
    Prints exactly length amount of characters to VWF buffer.

    Returns the tile data address where the start of updated character information resides.

    Length is stored in vwf_tiledata_run as a global variable.

    It is assumed that the caller will copy all relevant sections manually and length is up to the caller

    Implemented as a function-like macro to allow for inlining and optimization.
*/
#define ALIGN_POINTERS() \
do { \
    int i; \
    if (shift == 0) { \
        i = 0; \
    } else { \
        i = 1; \
        *tilemap_ptr = tile_id; \
        tilemap_ptr++; \
    } \
    int remaining = (32 - col) + vwf_col_start; \
    for (; i < remaining; i++) { \
        *tilemap_ptr = 0x0000; \
        tilemap_ptr++; \
    } \
    col = vwf_col_start; \
    shift = 0; \
    string_ptr++; \
    if (text_rendered && !text_prev_is_newline) { \
        run_width++; \
        advance_width++; \
        tile_id++; \
        tiledata_ptr += 16; \
    } \
} while (0)

/**
 * @brief Instantly renders a string to a local tilemap and tile graphics block in WRAM.
 * 
 * @param string       Pointer to the source null-terminated character string.
 * @param dest         Pointer to the target character graphics buffer in WRAM.
 * @param tilemap_dest Pointer to the target tilemap buffer in WRAM.
 * @param col_ext      Horizontal column bounds constraint.
 * @param row_ext      Vertical row bounds constraint.
 * @param id_offset    Base tile ID offset in VRAM.
 * @return The final length of the rendered string in bytes.
 */
uint16_t VwfEngine_PrintText(uint8_t * string, uint8_t * dest, uint8_t * tilemap_dest, int col_ext, int row_ext, int id_offset)
{
    VwfEngine_PrintText_Gradual_Setup(string, dest, tilemap_dest, col_ext, row_ext, id_offset, 896);
    VwfEngine_PrintText_Gradual(32767);
    return vwf_tiledata_run+16;
}

/**
 * @brief Configures textbox registers to prepare for a multi-frame gradual text reveal.
 * 
 * @param string       Pointer to the source null-terminated character string.
 * @param dest         Pointer to the target character graphics buffer in WRAM.
 * @param tilemap_dest Pointer to the target tilemap buffer in WRAM.
 * @param col_ext      Horizontal column bounds constraint.
 * @param row_ext      Vertical row bounds constraint.
 * @param id_offset    Base tile ID offset in VRAM.
 * @param tilemap_len  Maximum length of the tilemap buffer.
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

/**
 * @brief Reveals a specified number of characters in the current frame.
 * 
 * Processes control codes (newline, page breaks) and renders glyphs to buffers.
 * 
 * @param len The maximum number of characters to reveal in this pass.
 * @return The updated string pointer after processing.
 */
uint8_t * VwfEngine_PrintText_Gradual(int len)
{
    // Load state from global variables into local variables
    uint8_t * string_ptr = vwf_string_ptr;
    int shift = vwf_shift;
    int col = vwf_col;
    int row = vwf_row;
    uint16_t tile_id = vwf_tile_id;
    uint8_t * tiledata_ptr = vwf_tiledata_ptr;
    uint16_t * tilemap_ptr = vwf_tilemap_ptr;
    int run_width = 0;
    int advance_width = 0;
    bool text_rendered = vwf_text_rendered;
    bool text_prev_is_newline = vwf_text_prev_is_newline;

    uint8_t * write_ptr_start = tiledata_ptr;

    bool shift_overflow = false;

    if (!vwf_print_ongoing || (len != 32767)) // 32767 is instant
    {
        SoundInterface_PlayClip(STREAM_TYPEWRITER);
    }
    
    vwf_print_ongoing = true;

    for (int c = 0; c < len; c++)
    {
        if (*string_ptr != 0x00)
        {
            // Efficient implementation
            if (*string_ptr == '\n')
            {
                ALIGN_POINTERS();

                text_prev_is_newline = true;

                row++;
                
                continue;
            }
            else if (*string_ptr == '\r')
            {
                ALIGN_POINTERS();

                row = vwf_row_start;

                vwf_print_ongoing = false;

                break;
            }
            else if (col >= 32)
            {
                tilemap_ptr += (32-col)+vwf_col_start;

                row++;
                col = vwf_col_start;
                shift = 0;

                tile_id++;
                tiledata_ptr += 16;
            }
            
            text_prev_is_newline = false;

            if (row >= 28)
            {
                break;
            }

            text_rendered = true;

            uint8_t glyph_sel = *string_ptr;

            int width = const_ui_vwf_offsets[glyph_sel];

            uint8_t * glyph_ptr = (uint8_t *)&data_ui_vwf + ((uint16_t)glyph_sel << 4);

            if (shift_overflow)
            {
                run_width++;
                shift_overflow = false;
            }

            if (shift == 0)
            {
                // Copy the tile as is
                System_CopyBlock(glyph_ptr, tiledata_ptr, 16);
                if (run_width == 0)
                {
                    run_width = 1;
                }
            }
            else
            {
                uint16_t bitplane_mul = const_vwf_bitplane_mul[shift];

                VwfEngine_PrintText_Render(glyph_ptr, tiledata_ptr, bitplane_mul);

                if (run_width < 2)
                {
                    run_width = 2;
                }
            }

            *tilemap_ptr = tile_id;
            if (shift != 0)
            {
                *(tilemap_ptr+1) = tile_id+1;
            }

            shift += width;

            if (shift >= 8)
            {
                shift -= 8;
                tiledata_ptr += 16;
                tile_id++;
                tilemap_ptr++;
                col++;

                shift_overflow = true;
                advance_width++;
            }

            string_ptr++;
        }

        if (*string_ptr == 0x00)
        {
            vwf_print_finished = true;
            vwf_print_ongoing = false;
            break;
        }
    }

    if (shift_overflow) // One extra catch
    {
        run_width++;
        shift_overflow = false;
    }


    if (!vwf_print_ongoing) // Same rationale as the beginning one
    {
        SoundInterface_StopStream();
    }

    // Save local variables back to globals on exit
    vwf_string_ptr = string_ptr;
    vwf_shift = shift;
    vwf_col = col;
    vwf_row = row;
    vwf_tile_id = tile_id;
    vwf_tiledata_ptr = tiledata_ptr;
    vwf_tilemap_ptr = tilemap_ptr;
    vwf_text_rendered = text_rendered;
    vwf_text_prev_is_newline = text_prev_is_newline;

    vwf_run_width = run_width;
    vwf_advance_width = advance_width;

    vwf_tiledata_run = run_width << 4;
    vwf_tiledata_advance = advance_width << 4;
    vwf_tiledata_advance_vram = advance_width << 3;

    return write_ptr_start;
}

/**
 * @brief Core low-level bitplane rendering helper routine.
 * 
 * Shifts and projects glyph pixels onto the target destination format.
 * 
 * @param glyph_ptr [r4/r5] Pointer to the character glyph source data.
 * @param write_ptr [r6/r7] Pointer to the target destination buffer in WRAM.
 * @param mul       [a] Multiplier bitmask from `const_vwf_bitplane_mul`.
 */
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

/**
 * @brief Clears the text area to start displaying a new page of text.
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

/**
 * @brief Restores a section of the tilemap back to blank tiles.
 * 
 * @param ptr Pointer to the start of the tilemap section in WRAM.
 * @param len The number of tile entries to clear.
 */
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

const uint16_t const_vwf_bitplane_mul[8] = {
    0x0100, 0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002
};
