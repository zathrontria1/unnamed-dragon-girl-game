#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "hdma.h"

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

    HdmaEngine_GeneratePaletteTable((uint16_t *)&hdma_bgpalette_data[0], 0x42, 13, 1, 224, 0, BLENDMODE_ALPHA_TOWARDS_BLACK); // Water
    HdmaEngine_GeneratePaletteTable((uint16_t *)&hdma_windowbackground_data[0], 0x04, 4, 2, 48, 0, BLENDMODE_ALPHA_TOWARDS_BLACK); // UI message box using alpha towards black
    HdmaEngine_GeneratePaletteTable((uint16_t *)&hdma_windowbackground_data[1], 0x04, 4, 1, 224, 1, BLENDMODE_ALPHA_TOWARDS_BLACK); // UI full height using alpha towards black

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

void HdmaEngine_GeneratePaletteTable(uint16_t * table_ptr, uint16_t pal_start, uint16_t entries, int16_t intensity_change, int16_t height, uint16_t rate_delay, uint16_t blend_mode)
{
    int16_t temp_intensity = 0;

    int i = 0;

    int temp_intensity_counter = 0;

    for (; i < height; i += entries)
    {
        uint8_t temp_overflow = 0;

        for (int j = 0; j < entries; j++)
        {
            *table_ptr = (pal_start+j) << 8;

            int16_t temp_r;
            int16_t temp_g;
            int16_t temp_b;

            if (blend_mode == BLENDMODE_ALPHA_TOWARDS_BLACK)
            {
                if (temp_intensity > 31)
                {
                    temp_intensity = 31;
                }

                temp_r = ((shadow_cgram.entry[pal_start+j] & 0x001f) * (31 - temp_intensity)) / 31;
                temp_g = (((shadow_cgram.entry[pal_start+j] & 0x03e0) >> 5) * (31 - temp_intensity)) / 31;
                temp_b = (((shadow_cgram.entry[pal_start+j] & 0x7c00) >> 10) * (31 - temp_intensity)) / 31;
            }
            else
            {
                temp_r = (shadow_cgram.entry[pal_start+j] & 0x001f) + temp_intensity;
                temp_g = ((shadow_cgram.entry[pal_start+j] & 0x03e0) >> 5) + temp_intensity;
                temp_b = ((shadow_cgram.entry[pal_start+j] & 0x7c00) >> 10) + temp_intensity;
            }
            
            
            if (temp_r < 0)
            {
                temp_r = 0;
            }
            if (temp_r > 31)
            {
                temp_r = 31;
            }

            if (temp_g < 0)
            {
                temp_g = 0;
            }
            if (temp_g > 31)
            {
                temp_g = 31;
            }

            if (temp_b < 0)
            {
                temp_b = 0;
            }
            if (temp_b > 31)
            {
                temp_b = 31;
            }

            table_ptr++;

            *table_ptr = RGB5(temp_r, temp_g, temp_b);

            table_ptr++;

            if ((i+j+1) >= height)
            {
                temp_overflow = 1;
                break;
            }
        }

        if (temp_overflow == 1)
        {
            break;
        }

        temp_intensity_counter += entries;
        
        while (temp_intensity_counter >= (entries << rate_delay))
        {
            temp_intensity_counter -= (entries << rate_delay);
            temp_intensity += intensity_change;
        }
    }

    if (i + entries < 224) // only do this if there are enough spare entries to do it
    {
        // Fill the last entries with the resets
        for (int j = 0; j < entries; j++)
        {
            *table_ptr = (pal_start+j) << 8;
            table_ptr++;
            *table_ptr = shadow_cgram.entry[pal_start+j];
            table_ptr++;
        }
    }

    return;
}

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