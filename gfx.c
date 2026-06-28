#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "gfx.h"
#include "obj.h"
#include "map.h"

#include "math_int.h"

#include "snd.h"

#include "consts_snd.h"

uint16_t gfx_mosaic_layers;
int16_t gfx_mosaic_intensity;
int16_t gfx_mosaic_change;

int16_t gfx_cmath_change;

int16_t gfx_cmath_r;
int16_t gfx_cmath_g;
int16_t gfx_cmath_b;

/*
    Manage mosaic function

    mosaic layer is set directly
    mosaic intensity and change can be tweaked to adjust both speed and size at runtime too
*/
void Gfx_ProcessMosaic()
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

void Gfx_ProcessColorMath()
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

void Gfx_SetColorMath(int16_t r, int16_t g, int16_t b)
{
    gfx_cmath_r = r << 8;
    gfx_cmath_g = g << 8;
    gfx_cmath_b = b << 8;

    return;
}

/*
    Call to emit smoke effects.
*/
void Gfx_EmitSmoke(struct game_object * o)
{
    if (snd_firecrackle_timeout == 0)
    {
        SoundInterface_PlaySfx_Pre(o, SFX_ATK_FIRE_CRACKLE);

        snd_firecrackle_timeout = (8 / V_MUL);
    }

    // Only spawn objects every 8 frames...
    if (
        (((uint16_t)(system_frames_elapsed) & FX_SMOKE_INTERVAL) == FX_SMOKE_INTERVAL)
    )
    {
        int16_t temp_x = o->pos.x.lh.h;
        int16_t temp_y = o->pos.y.lh.h;
        int16_t k = ObjectSystem_InstantiateObject(OBJID_FX_SMOKE, temp_x, temp_y, 0);
        
        if (k >= 0)
        {
            struct game_object * q = &obj_general[k];

            int32_t temp = ((int32_t)Math_GetRandom_u16() - 16384);

            q->delta.x.a = temp;
            q->delta.y.a = -(V_S_ONE / 2);

            q->struct_data.npc_data.ttl = FX_SMOKE_TTL;
        }
    }

    return;
}
