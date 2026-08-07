#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "obj.h"

#include "map.h"
#include "hdma.h"
#include "gfx.h"

#if VBCC_ASM == 0
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
        hdma_scroll_data[temp_table_to_write][i] = temp_y + const_hdma_scroll_sine[temp_sine_select][hdma_scroll_sine_index + i];
    }

    hdma_scroll_select = temp_table_to_write;
    hdma_scroll_ptr = ADDR_LOWORD(&hdma_scroll_tables[hdma_scroll_select]);

    hdma_scroll_sine_index += (1 * V_MUL) >> 1;
    hdma_scroll_sine_index &= 31;

    return;
}

void HdmaEngine_UpdateColdataValues()
{
    uint16_t temp_table_to_write = (hdma_coldata_select + 1) & 0x01;

    if (!hdma_coldata_usegradient)
    {
        for (int i = 31; i >= 0; i--)
        {
            hdma_coldata_data[temp_table_to_write][i][0] = 0;
            hdma_coldata_data[temp_table_to_write][i][1] = 0;
            hdma_coldata_data[temp_table_to_write][i][2] = 0;
        }
    }
    else
    {
        uint16_t r_add = gfx_cmath_r >> 5;
        uint16_t g_add = gfx_cmath_g >> 5;
        uint16_t b_add = gfx_cmath_b >> 5;

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
    hdma_coldata_ptr = ADDR_LOWORD(&hdma_coldata_tables[hdma_coldata_select]);

    return;
}
#endif
