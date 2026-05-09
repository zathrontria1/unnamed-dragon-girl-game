#include <stdint.h>
#include <snes/console.h>

#include "vars.h"

#include "gfx.h"

/*
    Manage mosaic function

    mosaic layer is set directly
    mosaic intensity and change can be tweaked to adjust both speed and size at runtime too
*/
void gfx_process_mosaic()
{
    if (gfx_mosaic_change != 0)
    {
        gfx_mosaic_intensity += gfx_mosaic_change;

        if (gfx_mosaic_intensity < 0)
        {
            gfx_mosaic_intensity = 0;

            if (gfx_mosaic_change < 0)
            {
                gfx_mosaic_change = 0;
            }
        }
        else if (gfx_mosaic_intensity > 16)
        {
            gfx_mosaic_intensity = 16;

            if (gfx_mosaic_change > 0)
            {
                gfx_mosaic_change = 0;
            }
        }
    }

    if (gfx_mosaic_intensity == 0)
    {
        shadow_mosaic = 0x00;
        gfx_mosaic_layers = 0x0000;
    }
    else
    {
        #if VBCC_ASM == 1
            __asm(
                "\ta8\n"
                "\tsep #$20\n"
                "\tlda _gfx_mosaic_intensity\n"
                "\tdec\n"
                "\tasl\n"
                "\tasl\n"
                "\tasl\n"
                "\tasl\n"
                "\tora _gfx_mosaic_layers\n"
                "\tsta <_shadow_mosaic\n"
                "\ta16\n"
                "\trep #$20\n"
            );
        #else
            shadow_mosaic = (gfx_mosaic_layers | ((gfx_mosaic_intensity - 1) << 4));
        #endif
    }

    return;
}

void gfx_process_screen_cmath()
{
    if (gfx_cmath_change != 0)
    {
        gfx_cmath_r += gfx_cmath_change;
        gfx_cmath_g += gfx_cmath_change;
        gfx_cmath_b += gfx_cmath_change;

        if (gfx_cmath_change > 0)
        {
            if (gfx_cmath_r > (31 << 8)) gfx_cmath_r = (31 << 8);
            if (gfx_cmath_g > (31 << 8)) gfx_cmath_g = (31 << 8);
            if (gfx_cmath_b > (31 << 8)) gfx_cmath_b = (31 << 8);

            if ((gfx_cmath_r >= (31 << 8)) && (gfx_cmath_g >= (31 << 8)) && (gfx_cmath_b >= (31 << 8)))
            {
                gfx_cmath_change = 0;
            }
        }
        else if (gfx_cmath_change < 0)
        {
            if (gfx_cmath_r < 0) gfx_cmath_r = 0;
            if (gfx_cmath_g < 0) gfx_cmath_g = 0;
            if (gfx_cmath_b < 0) gfx_cmath_b = 0;

            if ((gfx_cmath_r | gfx_cmath_g | gfx_cmath_b) == 0x00)
            {
                gfx_cmath_change = 0;
            }
        }
    }

    // CGWSUB and CGADSUB shadows can be set directly
    if ((gfx_cmath_r | gfx_cmath_g | gfx_cmath_b) == 0x00)
    {
        shadow_coldata_r = 0x20;
        shadow_coldata_g = 0x40;
        shadow_coldata_b = 0x80;
    }
    else
    {
        shadow_coldata_r = 0x20 | (gfx_cmath_r >> 8);
        shadow_coldata_g = 0x40 | (gfx_cmath_g >> 8);
        shadow_coldata_b = 0x80 | (gfx_cmath_b >> 8);
    }

    return;
}

inline void gfx_cmath_set(int16_t r, int16_t g, int16_t b)
{
    gfx_cmath_r = r << 8;
    gfx_cmath_g = g << 8;
    gfx_cmath_b = b << 8;

    return;
}