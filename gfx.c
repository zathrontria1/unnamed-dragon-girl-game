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
        shadow_mosaic = (gfx_mosaic_layers | ((gfx_mosaic_intensity - 1) << 4));
    }

    return;
}
