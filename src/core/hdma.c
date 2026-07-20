#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "obj.h"

#include "map.h"
#include "hdma.h"
#include "gfx.h"

#include "ui.h"

struct hdma_indirect_table_entry hdma_bgpalette_tables[3];
uint16_t hdma_bgpalette_data[SCREEN_HEIGHT << 1];

struct hdma_indirect_table_entry hdma_windowbackground_tables[2][4];
uint16_t hdma_windowbackground_data[2][SCREEN_HEIGHT << 1];
uint16_t hdma_windowbackground_select;

ZP uint16_t hdma_use_gradient;
ZP uint16_t hdma_gradient_ptr;

// Scroll tables for v-scroll
struct hdma_indirect_table_entry hdma_scroll_tables[2][8];
uint16_t hdma_scroll_data[2][32];

uint16_t hdma_scroll_select;
ZP uint16_t hdma_scroll_ptr;
uint16_t hdma_scroll_sine_index;

bool hdma_coldata_usegradient; // If set, does the more complicated colour math version
struct hdma_indirect_table_entry hdma_coldata_tables[2][225];
uint16_t hdma_coldata_data[2][32][4]; // Deliberately use 2 bytes; highest 8 bits relevant

uint16_t hdma_coldata_select;
ZP uint16_t hdma_coldata_ptr;

// These are so large that they are larger than the available stack (1536 bytes > 1024), so they're defined globally
uint8_t hdma_cache_scaled_r[CACHE_PALETTE_ENTRIES * 64];
uint8_t hdma_cache_scaled_g[CACHE_PALETTE_ENTRIES * 64];
uint8_t hdma_cache_scaled_b[CACHE_PALETTE_ENTRIES * 64];

/**
 * @brief Performs initial setup of all HDMA tables and triggers the first value updates.
 */
void HdmaEngine_SetupHdma()
{
    hdma_scroll_select = 0;

    HdmaEngine_SetupPaletteHdma();
    HdmaEngine_SetupBgScrollHdma();
    HdmaEngine_SetupColdataHdma();

    HdmaEngine_UpdateBgScrollValues();
    HdmaEngine_UpdateBgScrollValues(); // Yes, run this twice, so both tables are populated

    HdmaEngine_UpdateColdataValues();
    HdmaEngine_UpdateColdataValues(); // Same as above
    return;
}

/**
 * @brief Configures CGRAM palette gradients (water & UI textboxes) on DMA channels 1, 2, and 6.
 */
void HdmaEngine_SetupPaletteHdma()
{
    // Is a bit of a misnomer now. Maybe needs a rename.
    REG_DMAP1 = 0x43;  // CGADD+CGDATA - Indirect, pattern 3
    REG_DMAP2 = 0x43; 
    REG_DMAP6 = 0x00; // TM

    REG_BBAD1 = (uint8_t)((uint32_t)&REG_CGADD); // CGADD followed by CGDATA
    REG_BBAD2 = (uint8_t)((uint32_t)&REG_CGADD);
    REG_BBAD6 = (uint8_t)((uint32_t)&REG_TM);

    REG_A1T1LH = (uint16_t)((uint32_t)&hdma_bgpalette_tables[0]);
    REG_A1T2LH = (uint16_t)((uint32_t)&hdma_windowbackground_tables[0]);
    REG_A1T6LH = (uint16_t)((uint32_t)&const_hdma_tm_msgbox[0]);

    REG_A1B1 = (uint8_t)((uint32_t)&hdma_bgpalette_tables[0] >> 16);
    REG_A1B2 = (uint8_t)((uint32_t)&hdma_windowbackground_tables[0] >> 16);
    REG_A1B6 = (uint8_t)((uint32_t)&const_hdma_tm_msgbox[0] >> 16);

    REG_DAS1LH = (uint16_t)((uint32_t)&hdma_bgpalette_data[0]);
    REG_DAS2LH = (uint16_t)((uint32_t)&hdma_windowbackground_data[0]);

    REG_DASB1 = (uint8_t)((uint32_t)&hdma_bgpalette_data[0] >> 16);
    REG_DASB2 = (uint8_t)((uint32_t)&hdma_windowbackground_data[0] >> 16);

    hdma_bgpalette_tables[0].count = 0x80 | 112;
    hdma_bgpalette_tables[0].addr = (uint16_t)((uint32_t)&hdma_bgpalette_data[0]);

    hdma_bgpalette_tables[1].count = 0x80 | 112;
    hdma_bgpalette_tables[1].addr = (uint16_t)((uint32_t)&hdma_bgpalette_data[0] + 448);

    hdma_bgpalette_tables[2].count = 0;
    
    // background UI window, textbox ver
    hdma_windowbackground_tables[0][0].count = (UI_MSGBOX_ML_START * 4);
    hdma_windowbackground_tables[0][0].addr = (uint16_t)((uint32_t)&hdma_windowbackground_data[0]);

    hdma_windowbackground_tables[0][1].count = (UI_MSGBOX_ML_START * 4);
    hdma_windowbackground_tables[0][1].addr = (uint16_t)((uint32_t)&hdma_windowbackground_data[0]);

    hdma_windowbackground_tables[0][2].count = 0x80 | ((UI_MSGBOX_HEIGHT * 8) + 4); // Text box height plus reset entries
    hdma_windowbackground_tables[0][2].addr = (uint16_t)((uint32_t)&hdma_windowbackground_data[0]);
    
    hdma_windowbackground_tables[0][3].count = 0;

    // Fullscreen ver
    hdma_windowbackground_tables[1][0].count = 0x80 | 112;
    hdma_windowbackground_tables[1][0].addr = (uint16_t)((uint32_t)&hdma_windowbackground_data[1][0]);

    hdma_windowbackground_tables[1][1].count = 0x80 | 112;
    hdma_windowbackground_tables[1][1].addr = (uint16_t)((uint32_t)&hdma_windowbackground_data[1][0] + 448);

    hdma_windowbackground_tables[1][2].count = 0;

    HdmaEngine_GeneratePaletteTable((uint16_t *)&hdma_bgpalette_data[0], 0x37, 6, RGB5(0,0,0), 16, 224); // Water
    HdmaEngine_GeneratePaletteTable((uint16_t *)&hdma_windowbackground_data[0], 0x04, 4, RGB5(0,0,0), 26, 48); // UI message box using alpha towards black
    HdmaEngine_GeneratePaletteTable((uint16_t *)&hdma_windowbackground_data[1], 0x04, 4, RGB5(0,0,0), 26, 224); // UI full height using alpha towards black

    hdma_gradient_ptr = (uint16_t)((uint32_t)&hdma_windowbackground_tables[0][0]);

    return;
}

/**
 * @brief Configures background vertical scroll sine-shaking on DMA channel 3.
 */
void HdmaEngine_SetupBgScrollHdma()
{
    REG_DMAP3 = 0x42;  // BG2HOFS - Indirect, pattern 2

    REG_BBAD3 = (uint8_t)((uint32_t)&REG_BG2VOFS); // One register, write twice

    REG_A1T3LH = (uint16_t)((uint32_t)&hdma_scroll_tables[0]);

    REG_A1B3 = (uint8_t)((uint32_t)&hdma_scroll_tables[0] >> 16);

    REG_DAS3LH = (uint16_t)((uint32_t)&hdma_scroll_data[0]);

    REG_DASB3 = (uint8_t)((uint32_t)&hdma_scroll_data[0] >> 16);

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 7; j++)
        {
            hdma_scroll_tables[i][j].count = 0x80 | 32;
            hdma_scroll_tables[i][j].addr = (uint16_t)((uint32_t)&hdma_scroll_data[i]);
        }

        hdma_scroll_tables[i][7].count = 0;
    }

    return;
}

/**
 * @brief Configures color math registers on DMA channel 4.
 */
void HdmaEngine_SetupColdataHdma()
{
    REG_DMAP4 = 0x40;  // COLDATA - Indirect, pattern 1

    REG_BBAD4 = (uint8_t)((uint32_t)&REG_COLDATA); // One register

    REG_A1T4LH = (uint16_t)((uint32_t)&hdma_coldata_tables[0]);

    REG_A1B4 = (uint8_t)((uint32_t)&hdma_coldata_tables[0] >> 16);

    REG_DAS4LH = (uint16_t)((uint32_t)&hdma_coldata_data[0]);

    REG_DASB4 = (uint8_t)((uint32_t)&hdma_coldata_data[0] >> 16);

    // The indirect table is 64 entries large.
    // Set up the tables so that the max intensity is the first entry (last data)
    // and go backwards, then mirror it.

    struct hdma_indirect_table_entry *table_base = &hdma_coldata_tables[0][0];
    uint16_t element_base_addr = (uint16_t)((uint32_t)&hdma_coldata_data[0][0][0]);

    for (int i = 0; i < 2; i++)
    {
        struct hdma_indirect_table_entry *table_ptr = table_base;
        uint16_t element_addr = element_base_addr;

        if (i == 1)
        {
            table_ptr += 225;
            element_addr += 256;
        }

        // For steps 0..31: incrementing phase (unrolled by 2)
        for (int j = 0; j < 16; j++)
        {
            // Step (even)
            table_ptr->count = 1;
            table_ptr->addr = element_addr + 1;
            table_ptr++;
            table_ptr->count = 1;
            table_ptr->addr = element_addr + 3;
            table_ptr++;
            table_ptr->count = 1;
            table_ptr->addr = element_addr + 5;
            table_ptr++;

            element_addr += 8;

            // Step (odd)
            table_ptr->count = 2;
            table_ptr->addr = element_addr + 1;
            table_ptr++;
            table_ptr->count = 1;
            table_ptr->addr = element_addr + 3;
            table_ptr++;
            table_ptr->count = 1;
            table_ptr->addr = element_addr + 5;
            table_ptr++;

            element_addr += 8;
        }
        element_addr -= 8;

        // For steps 32..63: decrementing phase (unrolled by 2)
        for (int j = 0; j < 16; j++)
        {
            // Step (even)
            table_ptr->count = 1;
            table_ptr->addr = element_addr + 1;
            table_ptr++;
            table_ptr->count = 1;
            table_ptr->addr = element_addr + 3;
            table_ptr++;
            table_ptr->count = 1;
            table_ptr->addr = element_addr + 5;
            table_ptr++;

            element_addr -= 8;

            // Step (odd)
            table_ptr->count = 2;
            table_ptr->addr = element_addr + 1;
            table_ptr++;
            table_ptr->count = 1;
            table_ptr->addr = element_addr + 3;
            table_ptr++;
            table_ptr->count = 1;
            table_ptr->addr = element_addr + 5;
            table_ptr++;

            element_addr -= 8;
        }

        // Set the last byte as a terminator
        table_ptr->count = 0;
    }

    return;
}

/**
 * @brief Updates scroll offsets in the active double-buffered background scroll table.
 */
void HdmaEngine_UpdateBgScrollValues()
{
    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\tlda _hdma_scroll_select\n"
            "\tinc\n"
            "\tand #$0001\n"
            "\tsta r0\n"
            "\txba\n"
            "\tlsr\n"
            "\tlsr\n" // Carry is cleared
            "\tadc #<_hdma_scroll_data\n"
            "\tsta r3\n"
            "\tlda #^_hdma_scroll_data\n"
            "\tsta r4\n"

            "\tlda _obj_player_active_fireballs\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tcmp #16\n"
            "\tbcc .fireball_in_limit\n"
                "\tlda #15\n"

            ".fireball_in_limit:\n"
            "\txba\n"
            "\tlsr\n"
            "\tsta r5\n"
            "\tlda _hdma_scroll_sine_index\n"
            "\tasl\n" // Carry is cleared
            "\tadc r5\n" 
            "\tadc #<_const_hdma_scroll_sine\n" // Carry is still clear
            "\tsta r5\n" // This table is located in near memory
            "\tlda #^_const_hdma_scroll_sine\n" // This table is located in near memory
            "\tsta r6\n" // This table is located in near memory

            "\tlda _bg_scroll_y+2\n"
            "\tdec\n"
            "\ttax\n" // Can be saved in X

            "\tldy #$003e\n"

            ".loop:\n"
            "\ttxa\n"
            "\tclc\n"
            "\tadc [r5],y\n"
            "\tsta [r3],y\n"
            "\tdey\n"
            "\tdey\n"
            "\tbpl .loop\n"

            "\tlda r0\n" 
            "\tsta _hdma_scroll_select\n"
            "\tbeq .scroll_no_offset\n"
                "\tlda #24\n"
            ".scroll_no_offset:\n"

            "\tclc\n"
            "\tadc #<_hdma_scroll_tables\n"
            "\tsta _hdma_scroll_ptr\n" // Must do last to avoid corruption
        );
    #else
        uint16_t temp_table_to_write = (hdma_scroll_select + 1) & 0x01;

        uint16_t temp_sine_select = obj_player_active_fireballs >> 2;
        if (temp_sine_select >= 16)
        {
            temp_sine_select = 15;
        }

        int16_t temp_y = bg_scroll_y.full.high.a - 1;
        
        for (int i = 0; i < 32; i++)
        {
            hdma_scroll_data[temp_table_to_write][i] = temp_y + const_hdma_scroll_sine[temp_sine_select][hdma_scroll_sine_index+i];
        }

        hdma_scroll_select = temp_table_to_write;
        hdma_scroll_ptr = (uint16_t)((uint32_t)&hdma_scroll_tables[hdma_scroll_select]);
    #endif
    
    hdma_scroll_sine_index += (1 * V_MUL) >> 1;

    while (hdma_scroll_sine_index >= 32)
    {
        hdma_scroll_sine_index -= 32;
    }

    return;
}

#if VBCC_ASM == 1
/**
 * @brief Updates color intensity steps in the active double-buffered color math table.
 */
NO_INLINE void HdmaEngine_UpdateColdataValues()
#else
/**
 * @brief Updates color intensity steps in the active double-buffered color math table.
 */
void HdmaEngine_UpdateColdataValues()
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\tlda _hdma_coldata_select\n"
            "\tinc\n"
            "\tand #$0001\n"
            "\tsta r0\n"
            "\txba\n" // x256
            "\tadc #<_hdma_coldata_data\n"
            "\tsta r3\n" // R pointer
            "\tadc #2\n"
            "\tsta r5\n" // G pointer
            "\tadc #2\n"
            "\tsta r7\n" // B pointer
            "\tlda #^_hdma_coldata_data\n"
            "\tsta r4\n"
            "\tsta r6\n"
            "\tsta r8\n" // Bank bytes

            "\tlda _hdma_coldata_usegradient\n"
            "\tand #$00ff\n"
            "\tbeq .coldata_is_zero\n"

            // Calculate what the added R, G, and B values should be
            "\tlda _gfx_cmath_r\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tlsr\n" // /32
            "\tsta r9\n" // R add

            "\tlda _gfx_cmath_g\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tsta r10\n" // G add
 
            "\tlda _gfx_cmath_b\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tsta r11\n" // B add

            // Check if RGB is valid
            "\tlda r9\n"
            "\tora r10\n"
            "\tora r11\n"
            "\tbne .start_coldata\n"
                ".coldata_is_zero:"
                // COLDATA is all zero. Use a special loop to write 0s to the entire table.
                "\tldy #$00fc\n" // The last B entry
                "\tlda #$0000\n"
                
                ".loop_coldata_zero:\n" 
                    "\tsta [r3],y\n"
                    "\tdey\n"
                    "\tdey\n"
                    "\tbpl .loop_coldata_zero\n"
                    "\tbra .end_coldata_write\n"

            ".start_coldata:\n" 

            // Write initial values
            "\tlda #$2000\n"
            "\tsta r12\n" // Running R
            "\tlda #$4000\n"
            "\tsta r13\n" // Running G
            "\tlda #$8000\n"
            "\tsta r14\n" // Running B

            // Start from the highest offset (entry 31)
            // Byte wise they'd be the offset 248, 250, 252
            "\tldy #$00f8\n" // Work backward so that once this becomes neg we can exit.

            // Put loop here
            ".loop_coldata:\n" 
                "\tlda r12\n" 
                "\tclc\n" 
                "\tadc r9\n"
                "\tsta [r3], y\n"
                "\tsta r12\n"

                "\tlda r13\n" 
                "\tadc r10\n"
                "\tsta [r5], y\n"
                "\tsta r13\n"

                "\tlda r14\n" 
                "\tadc r11\n"
                "\tsta [r7], y\n"
                "\tsta r14\n"

                "\ttya\n" 
                "\tsec\n" 
                "\tsbc #8\n"  // each data entry is 8 bytes wide.
                "\ttay\n" 
                "\tbpl .loop_coldata\n" 

            // End of loop
            ".end_coldata_write:\n" 

            "\tlda r0\n" 
            "\tsta _hdma_coldata_select\n"
            "\tbeq .coldata_no_offset\n"
                "\tlda #$02a3\n" // 225x3
            ".coldata_no_offset:\n"

            "\tclc\n"
            "\tadc #<_hdma_coldata_tables\n"
            "\tsta _hdma_coldata_ptr\n" // Must do last to avoid corruption
        );
    #else
        uint16_t temp_table_to_write = (hdma_coldata_select + 1) & 0x01;

        if (!hdma_coldata_usegradient) // Gradient mode disabled
        {
            for (int i = 31; i >= 0; i--)
            {
                hdma_coldata_data[temp_table_to_write][i][0] = 0; // Make all entries no-ops
                hdma_coldata_data[temp_table_to_write][i][1] = 0;
                hdma_coldata_data[temp_table_to_write][i][2] = 0;
            }
        }
        else
        {
            // Split out the coldata values... and divide them by 32
            uint16_t r_add = gfx_cmath_r >> 5;
            uint16_t g_add = gfx_cmath_g >> 5;
            uint16_t b_add = gfx_cmath_b >> 5;

            // It's a fade going from value of 0 all the way to max saturation
            uint16_t r = 0x2000;
            uint16_t g = 0x4000;
            uint16_t b = 0x8000;

            for (int i = 31; i >= 0; i--)
            {
                hdma_coldata_data[temp_table_to_write][i][0] = r;
                hdma_coldata_data[temp_table_to_write][i][1] = g;
                hdma_coldata_data[temp_table_to_write][i][2] = b;

                r += r_add;
                g += g_add;
                b += b_add;
            }
        }

        hdma_coldata_select = temp_table_to_write;
        hdma_coldata_ptr = (uint16_t)((uint32_t)&hdma_coldata_tables[hdma_coldata_select]);
    #endif

    return;
}

/**
 * @brief Safely enables active HDMA channels by making sure any activation is done during Vblank.
 */
void HdmaEngine_EnableHdma()
{
    // Check that we're out of vblank first...
    while((REG_HVBJOY & VBL_READY) == VBL_READY)
        ;

    // Then check if we entered vblank
    while((REG_HVBJOY & VBL_READY) == 0x00)
        ;

    
    REG_HDMAEN = HDMA_USED_CHANNELS_NORMAL;

    return;
}

/**
 * @brief Generates HDMA palette entries using fixed-point math and Bresenham interpolation.
 * 
 * Used to draw color transitions down the screen.
 * 
 * @param table_ptr    Target buffer in WRAM to write HDMA entries.
 * @param pal_start    Target CGRAM palette index to overwrite.
 * @param entries      Number of palette entries.
 * @param target_color Blend destination color in SNES RGB555 format.
 * @param alpha        Blend alpha coefficient (0 to 32).
 * @param height       Visual height constraint in scanlines.
 */
void HdmaEngine_GeneratePaletteTable(uint16_t * table_ptr, uint16_t pal_start, uint16_t entries, uint16_t target_color, uint16_t alpha, uint16_t height)
{
    // Cache target colors
    uint8_t cache_scaled_target_r[33];
    uint8_t cache_scaled_target_g[33];
    uint8_t cache_scaled_target_b[33];
    int Q, R, j;
    uint16_t val2;
    uint16_t temp_r2, temp_g2, temp_b2;
    uint16_t cgram_addr_start;
    uint16_t cgram_addr;
    uint16_t entries_shifted;
    uint16_t j_shifted;
    int y;
    int w;
    int k;

    // Split the target colour
    temp_r2 = (target_color & 0x001f);
    temp_g2 = (target_color & 0x03e0) >> 5;
    temp_b2 = (target_color & 0x7c00) >> 10;

    for (j = 0; j < entries; j++)
    {
        uint16_t color = shadow_cgram.entry[pal_start + j];
        uint16_t r1 = color & 0x001f;
        uint16_t g1 = (color & 0x03e0) >> 5;
        uint16_t b1 = (color & 0x7c00) >> 10;
        uint16_t cache_j_shifted = j << 6;

        for (w = 0; w <= 32; w++)
        {
            hdma_cache_scaled_r[cache_j_shifted | w] = (r1 * w) >> 5;
            hdma_cache_scaled_g[cache_j_shifted | w] = (g1 * w) >> 5;
            hdma_cache_scaled_b[cache_j_shifted | w] = (b1 * w) >> 5;
        }
    }

    if (target_color != 0)
    {
        for (w = 0; w <= 32; w++)
        {
            cache_scaled_target_r[w] = (temp_r2 * w) >> 5;
            cache_scaled_target_g[w] = (temp_g2 * w) >> 5;
            cache_scaled_target_b[w] = (temp_b2 * w) >> 5;
        }
    }

    // Initialize Bresenham / fractional step parameters for the weight ramp
    // Q represents the weight value (starts at 32, decreases to 0)
    // R is the division remainder
    // val2 = Q_inv * alpha (starts at 0, increases by alpha whenever Q decrements / Q_inv increments)
    Q = 32;
    R = 0;
    val2 = 0;

    cgram_addr_start = pal_start << 8;
    cgram_addr = cgram_addr_start;
    entries_shifted = entries << 6;
    j_shifted = 0;

    for (y = 0; y < height; y++)
    {
        uint16_t temp_weight_2, temp_weight_1;
        uint16_t temp_r, temp_g, temp_b;
        uint16_t offset;

        // Write the CGRAM address
        *table_ptr = cgram_addr;
        table_ptr++;

        // temp_weight_2 = (Q_inv * alpha) >> 5;
        temp_weight_2 = val2 >> 5;
        temp_weight_1 = 32 - temp_weight_2;

        offset = j_shifted | temp_weight_1;

        // Weight them
        if (target_color == 0)
        {
            temp_r = hdma_cache_scaled_r[offset];
            temp_g = hdma_cache_scaled_g[offset];
            temp_b = hdma_cache_scaled_b[offset];
        }
        else
        {
            temp_r = hdma_cache_scaled_r[offset] + cache_scaled_target_r[temp_weight_2];
            temp_g = hdma_cache_scaled_g[offset] + cache_scaled_target_g[temp_weight_2];
            temp_b = hdma_cache_scaled_b[offset] + cache_scaled_target_b[temp_weight_2];
        }

        *table_ptr = RGB5(temp_r, temp_g, temp_b);
        table_ptr++;

        // Update step parameters for the next iteration (y + 1)
        R -= 32;
        while (R < 0)
        {
            R += height;
            Q--;
            val2 += alpha;
        }

        // Increment and wrap palette index
        cgram_addr += 256;
        j_shifted += 64;
        if (j_shifted == entries_shifted)
        {
            j_shifted = 0;
            cgram_addr = cgram_addr_start;
        }
    }

    if (height < 223 - entries)
    {
        // Write palette restore entries
        uint16_t restore_addr = cgram_addr_start;
        for (k = 0; k < entries; k++)
        {
            *table_ptr = restore_addr;
            table_ptr++;
            *table_ptr = shadow_cgram.entry[pal_start + k];
            table_ptr++;
            restore_addr += 256;
        }
    }

    return;
}

/**
 * @brief Selects active HDMA channel bits in `shadow_hdmaen` based on the screen/dialog layout.
 */
void HdmaEngine_SetHdmaShadow()
{
    if (hdma_use_gradient == 0xffff)
    {
        if (ui_in_subscreen)
        {
            shadow_hdmaen = HDMA_USED_CHANNELS_SUBSCREEN;
        }
        else
        {
            shadow_hdmaen = HDMA_USED_CHANNELS_MSGBOX;
        }
    }
    else if (hdma_use_gradient == 0)
    {
        ; // Do not interfere with HDMA
    }
    else 
    {
        shadow_hdmaen = HDMA_USED_CHANNELS_NORMAL;
    }

    return;
}

const uint8_t const_hdma_tm_msgbox[] = 
{
    UI_MSGBOX_ML_START * 4, TM_MODE1, 
    UI_MSGBOX_ML_START * 4, TM_MODE1, 
    UI_MSGBOX_HEIGHT * 8, TM_MODE1_MSGBOX, 
    1, TM_MODE1, 
    0, 
};


// sine offsets, intensity 0-15, 2 cycles each
const int16_t const_hdma_scroll_sine[16][64] = 
{
    {
     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,
     0 },
    {
     0,     0,     0,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,
     0,     0,     0,     0,     0,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,     0,     0,     0,     0,     0,
     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     0,     0,     0,
     0,     0,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,     0,
     0 },
    {
     0,     0,     1,     1,     1,     2,     2,
     2,     2,     2,     2,     2,     1,     1,
     1,     0,     0,     0,    -1,    -1,    -1,
    -2,    -2,    -2,    -2,    -2,    -2,    -2,
    -1,    -1,    -1,     0,     0,     0,     1,
     1,     1,     2,     2,     2,     2,     2,
     2,     2,     1,     1,     1,     0,     0,
     0,    -1,    -1,    -1,    -2,    -2,    -2,
    -2,    -2,    -2,    -2,    -1,    -1,    -1,
     0 },
    {
     0,     1,     1,     2,     2,     2,     3,
     3,     3,     3,     3,     2,     2,     2,
     1,     1,     0,    -1,    -1,    -2,    -2,
    -2,    -3,    -3,    -3,    -3,    -3,    -2,
    -2,    -2,    -1,    -1,     0,     1,     1,
     2,     2,     2,     3,     3,     3,     3,
     3,     2,     2,     2,     1,     1,     0,
    -1,    -1,    -2,    -2,    -2,    -3,    -3,
    -3,    -3,    -3,    -2,    -2,    -2,    -1,
    -1 },

    {
     0,     1,     2,     2,     3,     3,     4,
     4,     4,     4,     4,     3,     3,     2,
     2,     1,     0,    -1,    -2,    -2,    -3,
    -3,    -4,    -4,    -4,    -4,    -4,    -3,
    -3,    -2,    -2,    -1,     0,     1,     2,
     2,     3,     3,     4,     4,     4,     4,
     4,     3,     3,     2,     2,     1,     0,
    -1,    -2,    -2,    -3,    -3,    -4,    -4,
    -4,    -4,    -4,    -3,    -3,    -2,    -2,
    -1 },
    {
     0,     1,     2,     3,     4,     4,     5,
     5,     5,     5,     5,     4,     4,     3,
     2,     1,     0,    -1,    -2,    -3,    -4,
    -4,    -5,    -5,    -5,    -5,    -5,    -4,
    -4,    -3,    -2,    -1,     0,     1,     2,
     3,     4,     4,     5,     5,     5,     5,
     5,     4,     4,     3,     2,     1,     0,
    -1,    -2,    -3,    -4,    -4,    -5,    -5,
    -5,    -5,    -5,    -4,    -4,    -3,    -2,
    -1 },
    {
     0,     1,     2,     3,     4,     5,     6,
     6,     6,     6,     6,     5,     4,     3,
     2,     1,     0,    -1,    -2,    -3,    -4,
    -5,    -6,    -6,    -6,    -6,    -6,    -5,
    -4,    -3,    -2,    -1,     0,     1,     2,
     3,     4,     5,     6,     6,     6,     6,
     6,     5,     4,     3,     2,     1,     0,
    -1,    -2,    -3,    -4,    -5,    -6,    -6,
    -6,    -6,    -6,    -5,    -4,    -3,    -2,
    -1 },
    {
     0,     1,     3,     4,     5,     6,     6,
     7,     7,     7,     6,     6,     5,     4,
     3,     1,     0,    -1,    -3,    -4,    -5,
    -6,    -6,    -7,    -7,    -7,    -6,    -6,
    -5,    -4,    -3,    -1,     0,     1,     3,
     4,     5,     6,     6,     7,     7,     7,
     6,     6,     5,     4,     3,     1,     0,
    -1,    -3,    -4,    -5,    -6,    -6,    -7,
    -7,    -7,    -6,    -6,    -5,    -4,    -3,
    -1 },

    {
     0,     2,     3,     4,     6,     7,     7,
     8,     8,     8,     7,     7,     6,     4,
     3,     2,     0,    -2,    -3,    -4,    -6,
    -7,    -7,    -8,    -8,    -8,    -7,    -7,
    -6,    -4,    -3,    -2,     0,     2,     3,
     4,     6,     7,     7,     8,     8,     8,
     7,     7,     6,     4,     3,     2,     0,
    -2,    -3,    -4,    -6,    -7,    -7,    -8,
    -8,    -8,    -7,    -7,    -6,    -4,    -3,
    -2 },
    {
     0,     2,     3,     5,     6,     7,     8,
     9,     9,     9,     8,     7,     6,     5,
     3,     2,     0,    -2,    -3,    -5,    -6,
    -7,    -8,    -9,    -9,    -9,    -8,    -7,
    -6,    -5,    -3,    -2,     0,     2,     3,
     5,     6,     7,     8,     9,     9,     9,
     8,     7,     6,     5,     3,     2,     0,
    -2,    -3,    -5,    -6,    -7,    -8,    -9,
    -9,    -9,    -8,    -7,    -6,    -5,    -3,
    -2 },
    {
     0,     2,     4,     6,     7,     8,     9,
    10,    10,    10,     9,     8,     7,     6,
     4,     2,     0,    -2,    -4,    -6,    -7,
    -8,    -9,   -10,   -10,   -10,    -9,    -8,
    -7,    -6,    -4,    -2,     0,     2,     4,
     6,     7,     8,     9,    10,    10,    10,
     9,     8,     7,     6,     4,     2,     0,
    -2,    -4,    -6,    -7,    -8,    -9,   -10,
   -10,   -10,    -9,    -8,    -7,    -6,    -4,
    -2 },
    {
     0,     2,     4,     6,     8,     9,    10,
    11,    11,    11,    10,     9,     8,     6,
     4,     2,     0,    -2,    -4,    -6,    -8,
    -9,   -10,   -11,   -11,   -11,   -10,    -9,
    -8,    -6,    -4,    -2,     0,     2,     4,
     6,     8,     9,    10,    11,    11,    11,
    10,     9,     8,     6,     4,     2,     0,
    -2,    -4,    -6,    -8,    -9,   -10,   -11,
   -11,   -11,   -10,    -9,    -8,    -6,    -4,
    -2 },
    {
     0,     2,     5,     7,     8,    10,    11,
    12,    12,    12,    11,    10,     8,     7,
     5,     2,     0,    -2,    -5,    -7,    -8,
   -10,   -11,   -12,   -12,   -12,   -11,   -10,
    -8,    -7,    -5,    -2,     0,     2,     5,
     7,     8,    10,    11,    12,    12,    12,
    11,    10,     8,     7,     5,     2,     0,
    -2,    -5,    -7,    -8,   -10,   -11,   -12,
   -12,   -12,   -11,   -10,    -8,    -7,    -5,
    -2 },
    {
     0,     3,     5,     7,     9,    11,    12,
    13,    13,    13,    12,    11,     9,     7,
     5,     3,     0,    -3,    -5,    -7,    -9,
   -11,   -12,   -13,   -13,   -13,   -12,   -11,
    -9,    -7,    -5,    -3,     0,     3,     5,
     7,     9,    11,    12,    13,    13,    13,
    12,    11,     9,     7,     5,     3,     0,
    -3,    -5,    -7,    -9,   -11,   -12,   -13,
   -13,   -13,   -12,   -11,    -9,    -7,    -5,
    -3 },
    {
     0,     3,     5,     8,    10,    12,    13,
    14,    14,    14,    13,    12,    10,     8,
     5,     3,     0,    -3,    -5,    -8,   -10,
   -12,   -13,   -14,   -14,   -14,   -13,   -12,
   -10,    -8,    -5,    -3,     0,     3,     5,
     8,    10,    12,    13,    14,    14,    14,
    13,    12,    10,     8,     5,     3,     0,
    -3,    -5,    -8,   -10,   -12,   -13,   -14,
   -14,   -14,   -13,   -12,   -10,    -8,    -5,
    -3 },
    {
     0,     3,     6,     8,    11,    12,    14,
    15,    15,    15,    14,    12,    11,     8,
     6,     3,     0,    -3,    -6,    -8,   -11,
   -12,   -14,   -15,   -15,   -15,   -14,   -12,
   -11,    -8,    -6,    -3,     0,     3,     6,
     8,    11,    12,    14,    15,    15,    15,
    14,    12,    11,     8,     6,     3,     0,
    -3,    -6,    -8,   -11,   -12,   -14,   -15,
   -15,   -15,   -14,   -12,   -11,    -8,    -6,
    -3 },
};