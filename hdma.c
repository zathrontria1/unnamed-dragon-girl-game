#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "obj.h"

#include "map.h"
#include "hdma.h"

struct hdma_indirect_table_entry hdma_bgpalette_tables[3];
uint16_t hdma_bgpalette_data[448];

struct hdma_indirect_table_entry hdma_windowbackground_tables[2][4];
uint16_t hdma_windowbackground_data[2][448];
uint16_t hdma_windowbackground_select;

uint16_t hdma_scroll_data[2][32];
uint16_t hdma_scroll_select;
ZP uint16_t hdma_scroll_ptr;
uint16_t hdma_scroll_sine_index;

ZP uint16_t hdma_use_gradient;
ZP uint16_t hdma_gradient_ptr;

// Scroll tables for v-scroll
struct hdma_indirect_table_entry hdma_scroll_tables[2][8];

/*
    Setup all HDMA and their tables
*/
void HdmaEngine_SetupHdma()
{
    hdma_scroll_select = 0;

    HdmaEngine_SetupPaletteHdma();
    HdmaEngine_SetupBgScrollHdma();
    HdmaEngine_UpdateBgScrollValues();
    HdmaEngine_UpdateBgScrollValues(); // Yes, run this twice, so both tables are populated

    return;
}

/*
    Sets up a gradient to half brightness effect on the water subpalette via HDMA tables
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

    HdmaEngine_GeneratePaletteTable((uint16_t *)&hdma_bgpalette_data[0], 0x42, 13, RGB5(0,0,0), 26, 224); // Water
    HdmaEngine_GeneratePaletteTable((uint16_t *)&hdma_windowbackground_data[0], 0x04, 4, RGB5(0,0,0), 26, 48); // UI message box using alpha towards black
    HdmaEngine_GeneratePaletteTable((uint16_t *)&hdma_windowbackground_data[1], 0x04, 4, RGB5(0,0,0), 26, 224); // UI full height using alpha towards black

    hdma_gradient_ptr = (uint16_t)((uint32_t)&hdma_windowbackground_tables[0][0]);

    return;
}

/*
    Same deal with background scroll HDMA
*/
void HdmaEngine_SetupBgScrollHdma()
{
    REG_DMAP3 = 0x42;  // BG2HOFS - Indirect, pattern 2

    REG_BBAD3 = (uint8_t)((uint32_t)&REG_BG2VOFS); // One register, write twice

    REG_A1T3LH = (uint16_t)((uint32_t)&hdma_scroll_tables[0]);

    REG_A1B3 = (uint8_t)((uint32_t)&hdma_scroll_tables[0] >> 16);

    REG_DAS3LH = (uint16_t)((uint32_t)&hdma_scroll_data[0]);

    REG_DASB3 = (uint8_t)((uint32_t)&hdma_scroll_data[0] >> 16);

    hdma_scroll_tables[0][0].count = 0x80 | 32;
    hdma_scroll_tables[0][0].addr = (uint16_t)((uint32_t)&hdma_scroll_data[0]);
    hdma_scroll_tables[0][1].count = 0x80 | 32;
    hdma_scroll_tables[0][1].addr = (uint16_t)((uint32_t)&hdma_scroll_data[0]);
    hdma_scroll_tables[0][2].count = 0x80 | 32;
    hdma_scroll_tables[0][2].addr = (uint16_t)((uint32_t)&hdma_scroll_data[0]);
    hdma_scroll_tables[0][3].count = 0x80 | 32;
    hdma_scroll_tables[0][3].addr = (uint16_t)((uint32_t)&hdma_scroll_data[0]);
    hdma_scroll_tables[0][4].count = 0x80 | 32;
    hdma_scroll_tables[0][4].addr = (uint16_t)((uint32_t)&hdma_scroll_data[0]);
    hdma_scroll_tables[0][5].count = 0x80 | 32;
    hdma_scroll_tables[0][5].addr = (uint16_t)((uint32_t)&hdma_scroll_data[0]);
    hdma_scroll_tables[0][6].count = 0x80 | 32;
    hdma_scroll_tables[0][6].addr = (uint16_t)((uint32_t)&hdma_scroll_data[0]);
    hdma_scroll_tables[0][7].count = 0;

    hdma_scroll_tables[1][0].count = 0x80 | 32;
    hdma_scroll_tables[1][0].addr = (uint16_t)((uint32_t)&hdma_scroll_data[1]);
    hdma_scroll_tables[1][1].count = 0x80 | 32;
    hdma_scroll_tables[1][1].addr = (uint16_t)((uint32_t)&hdma_scroll_data[1]);
    hdma_scroll_tables[1][2].count = 0x80 | 32;
    hdma_scroll_tables[1][2].addr = (uint16_t)((uint32_t)&hdma_scroll_data[1]);
    hdma_scroll_tables[1][3].count = 0x80 | 32;
    hdma_scroll_tables[1][3].addr = (uint16_t)((uint32_t)&hdma_scroll_data[1]);
    hdma_scroll_tables[1][4].count = 0x80 | 32;
    hdma_scroll_tables[1][4].addr = (uint16_t)((uint32_t)&hdma_scroll_data[1]);
    hdma_scroll_tables[1][5].count = 0x80 | 32;
    hdma_scroll_tables[1][5].addr = (uint16_t)((uint32_t)&hdma_scroll_data[1]);
    hdma_scroll_tables[1][6].count = 0x80 | 32;
    hdma_scroll_tables[1][6].addr = (uint16_t)((uint32_t)&hdma_scroll_data[1]);
    hdma_scroll_tables[1][7].count = 0;

    return;
}

/*
    Call to update background scroll values for the HDMA table.
    The table values are double-buffered, so also do a table pointer flip here.
*/
void HdmaEngine_UpdateBgScrollValues()
{
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

    hdma_scroll_sine_index += (1 * V_MUL) >> 1;

    while (hdma_scroll_sine_index >= 32)
    {
        hdma_scroll_sine_index -= 32;
    }

    return;
}

/*
    Use this helper function to ensure that HDMAEN is always written at the start of vblank without relying on NMI or interrupts.

    It's safe to directly disable HDMA anytime, so no equivalent function is provided for the reverse.
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

/*
    Generate HDMA palette entries using fixed point math. Slower, but can cleanly map to any height

    target_color is in SNES RGB555 format
    alpha is 0-32 unsigned integer
*/
void HdmaEngine_GeneratePaletteTable(uint16_t * table_ptr, uint16_t pal_start, uint16_t entries, uint16_t target_color, uint16_t alpha, uint16_t height)
{
    bool temp_overflow = false;

    // Split the target colour
    uint16_t temp_r2 = (target_color & 0x001f);
    uint16_t temp_g2 = (target_color & 0x03e0) >> 5;
    uint16_t temp_b2 = (target_color & 0x7c00) >> 10;

    for (int i = 0; i < height; i += entries)
    {
        for (int j = 0; j < entries; j++)
        {
            // Write the CGRAM address
            *table_ptr = (pal_start+j) << 8;

            // Fetch the base colour
            uint16_t temp_r1 = (shadow_cgram.entry[pal_start+j] & 0x001f);
            uint16_t temp_g1 = (shadow_cgram.entry[pal_start+j] & 0x03e0) >> 5;
            uint16_t temp_b1 = (shadow_cgram.entry[pal_start+j] & 0x7c00) >> 10;

            // Calculate weights
            uint16_t temp_weight_adjust = ((height - (i+j)) << 5) / height;

            uint16_t temp_weight_2 = ((32 - temp_weight_adjust) * alpha) >> 5;

            uint16_t temp_weight_1 = 32 - temp_weight_2;

            // Weight them
            uint16_t temp_r = ((temp_r1 * temp_weight_1) + (temp_r2 * temp_weight_2)) >> 5;
            uint16_t temp_g = ((temp_g1 * temp_weight_1) + (temp_g2 * temp_weight_2)) >> 5;
            uint16_t temp_b = ((temp_b1 * temp_weight_1) + (temp_b2 * temp_weight_2)) >> 5;

            // Make sure the value doesn't overflow on the channel
            if (temp_r > 31)
            {
                temp_r = 31;
            }
            if (temp_g > 31)
            {
                temp_g = 31;
            }
            if (temp_b > 31)
            {
                temp_b = 31;
            }

            // Save the colour value
            table_ptr++;

            *table_ptr = RGB5(temp_r, temp_g, temp_b);

            table_ptr++;

            if ((i+j+1) >= height)
            {
                // If this is true, the next entry will go out of range, and terminate the table now
                temp_overflow = true;
                break;
            }
        }

        if (temp_overflow)
        {
            break;
        }
    }

    if (height < 223-entries)
    {
        // Write palette restore entries
        for (int j = 0; j < entries; j++)
        {
            // Write the CGRAM address
            *table_ptr = (pal_start+j) << 8;

            table_ptr++;

            *table_ptr = shadow_cgram.entry[pal_start+j];
            
            table_ptr++;
        }
    }

    return;
}

// sine offsets, intensity 0-15, 2 cycles each
NEAR const int16_t const_hdma_scroll_sine[16][64] = 
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