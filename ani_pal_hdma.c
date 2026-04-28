#include <stdint.h>
#include <snes/console.h>
#include "ani_pal_hdma.h"

#include "vars.h"

/*
    Sets up a gradient to half brightness effect on the water subpalette via HDMA tables
*/
void ani_pal_hdma_setup()
{
    // Is a bit of a misnomer now. Maybe needs a rename.
    REG_DMAP1 = 0x43;  // CGADD+CGDATA - Indirect, pattern 3
    REG_DMAP2 = 0x43; 
    REG_DMAP6 = 0x00; // TM

    REG_BBAD1 = (uint8_t)((uint32_t)&REG_CGADD); // CGADD followed by CGDATA
    REG_BBAD2 = (uint8_t)((uint32_t)&REG_CGADD);
    REG_BBAD6 = (uint8_t)((uint32_t)&REG_TM);

    REG_A1T1LH = (uint16_t)((uint32_t)&hdma_indirect_tables[0]);
    REG_A1T2LH = (uint16_t)((uint32_t)&hdma_indirect_tables[1]);
    REG_A1T6LH = (uint16_t)((uint32_t)&const_hdma_tm_msgbox[0]);

    REG_A1B1 = (uint8_t)((uint32_t)&hdma_indirect_tables[0] >> 16);
    REG_A1B2 = (uint8_t)((uint32_t)&hdma_indirect_tables[1] >> 16);
    REG_A1B6 = (uint8_t)((uint32_t)&const_hdma_tm_msgbox[0] >> 16);

    REG_DAS1LH = (uint16_t)((uint32_t)&hdma_indirect_data[0]);
    REG_DAS2LH = (uint16_t)((uint32_t)&hdma_indirect_data[1]);

    REG_DASB1 = (uint8_t)((uint32_t)&hdma_indirect_data[0] >> 16);
    REG_DASB2 = (uint8_t)((uint32_t)&hdma_indirect_data[1] >> 16);

    hdma_indirect_tables[0][0].count = 0x80 | 112;
    hdma_indirect_tables[0][0].addr = (uint16_t)((uint32_t)&hdma_indirect_data[0]);

    hdma_indirect_tables[0][1].count = 0x80 | 112;
    hdma_indirect_tables[0][1].addr = (uint16_t)((uint32_t)&hdma_indirect_data[0] + 448);

    hdma_indirect_tables[0][2].count = 0;

    hdma_indirect_tables[1][0].count = (UI_MSGBOX_ML_START * 4);
    hdma_indirect_tables[1][0].addr = (uint16_t)((uint32_t)&hdma_indirect_data[1]);

    hdma_indirect_tables[1][1].count = (UI_MSGBOX_ML_START * 4);
    hdma_indirect_tables[1][1].addr = (uint16_t)((uint32_t)&hdma_indirect_data[1]);

    hdma_indirect_tables[1][2].count = 0x80 | ((UI_MSGBOX_HEIGHT * 8) + 4); // Text box height plus reset entries
    hdma_indirect_tables[1][2].addr = (uint16_t)((uint32_t)&hdma_indirect_data[1]);
    
    hdma_indirect_tables[1][3].count = 0;

    ani_pal_hdma_make_table((uint16_t *)&hdma_indirect_data[0], 0x42, 13, 1, 224, 0, BLENDMODE_ALPHA_TOWARDS_BLACK); // Water
    ani_pal_hdma_make_table((uint16_t *)&hdma_indirect_data[1], 0x04, 4, 2, 48, 0, BLENDMODE_ALPHA_TOWARDS_BLACK); // UI message box using alpha towards black
    //ani_pal_hdma_make_table((uint16_t *)&hdma_indirect_data[1], 0x22, 4, -1, 16, 1, BLENDMODE_ADDSUB); // UI message box using subtraction

    return;
}

/*
    Use this helper function to ensure that HDMAEN is always written at the start of vblank without relying on NMI or interrupts.

    It's safe to directly disable HDMA anytime, so no equivalent function is provided for the reverse.
*/
void ani_pal_hdma_enable()
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

void ani_pal_hdma_make_table(uint16_t * table_ptr, uint16_t pal_start, uint16_t entries, int16_t intensity_change, int16_t height, uint16_t rate_delay, uint16_t blend_mode)
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
            else if (temp_r > 31)
            {
                temp_r = 31;
            }

            if (temp_g < 0)
            {
                temp_g = 0;
            }
            else if (temp_g > 31)
            {
                temp_g = 31;
            }

            if (temp_b < 0)
            {
                temp_b = 0;
            }
            else if (temp_b > 31)
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
        if (temp_intensity_counter >= (entries << rate_delay))
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