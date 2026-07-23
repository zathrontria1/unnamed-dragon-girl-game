#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "gfx.h"
#include "obj.h"
#include "map.h"
#include "hdma.h"

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

#define GFX_SMOKE_QUEUE_MAX_COUNT OBJ_GENERAL_MAX_COUNT

static pos_lh32_t gfx_smoke_queue[GFX_SMOKE_QUEUE_MAX_COUNT];
static uint8_t gfx_smoke_queue_first;
static uint8_t gfx_smoke_queue_count;

void Gfx_ResetSmoke()
{
    gfx_smoke_queue_first = 0;
    gfx_smoke_queue_count = 0;

    return;
}

/**
 * @brief Attempts to spawn one queued smoke particle for the current frame.
 */
void Gfx_ProcessSmoke()
{
    if (gfx_smoke_queue_count == 0)
    {
        return;
    }

    if (obj_first_available == 0xffff)
    {
        Gfx_ResetSmoke();
        return;
    }

    pos_lh32_t * request = &gfx_smoke_queue[gfx_smoke_queue_first];
    int16_t k = ObjectSystem_InstantiateObject(OBJID_FX_SMOKE, request->l, request->h, 0);

    if (k >= 0)
    {
        struct game_object * q = &obj_general[k];
        int32_t temp = ((int32_t)Math_GetRandom_u16() - 16384);

        q->delta.x.a = temp;
        q->delta.y.a = -V_S_ONE;
        q->struct_data.npc_data.ttl = FX_SMOKE_TTL;

        gfx_smoke_queue_first++;
        if (gfx_smoke_queue_first == GFX_SMOKE_QUEUE_MAX_COUNT)
        {
            gfx_smoke_queue_first = 0;
        }
        gfx_smoke_queue_count--;
    }

    return;
}

/**
 * @brief Advances active mosaic scaling transitions and updates `shadow_mosaic`.
 */
void Gfx_ProcessMosaic()
{
    if (gfx_mosaic_change != 0)
    {
        gfx_mosaic_intensity += (gfx_mosaic_change << 8);

        if (gfx_mosaic_intensity < 0)
        {
            gfx_mosaic_intensity = 0;

            if (gfx_mosaic_change < 0)
            {
                gfx_mosaic_change = 0;
            }
        }
        else if (gfx_mosaic_intensity > 0x0f00)
        {
            gfx_mosaic_intensity = 0x0f00;

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
        // Don't forget to -1 since hardware values 0-15 correspond to actual values 1-16. To achieve "mosaic off", disable all enabled layers instead.
        uint8_t mosaic_intensity = ((gfx_mosaic_intensity >> 8) & 0x0f) - 1;
        shadow_mosaic = (gfx_mosaic_layers & 0x0f) | (mosaic_intensity << 4);
    }

    return;
}

/**
 * @brief Advances active color math transitions and updates COLDATA registers in shadow WRAM.
 */
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

        hdma_coldata_usegradient = false;
    }
    else
    {
        shadow_coldata_r = 0x20 | (gfx_cmath_r >> 8);
        shadow_coldata_g = 0x40 | (gfx_cmath_g >> 8);
        shadow_coldata_b = 0x80 | (gfx_cmath_b >> 8);
    }

    return;
}

/**
 * @brief Immediately sets target RGB color math values and enables gradient modes.
 * 
 * @param r        Red intensity target (0 to 31).
 * @param g        Green intensity target (0 to 31).
 * @param b        Blue intensity target (0 to 31).
 * @param gradient Enable HDMA color math gradient transitions down scanlines.
 */
void Gfx_SetColorMath(int16_t r, int16_t g, int16_t b, bool gradient)
{
    gfx_cmath_r = r << 8;
    gfx_cmath_g = g << 8;
    gfx_cmath_b = b << 8;

    hdma_coldata_usegradient = gradient;

    return;
}

/**
 * @brief Spawns smoke particles centered on a game object's position with random velocity offsets.
 * 
 * @param o      Pointer to the game object emitting smoke.
 * @param offset Vertical Y pixel offset relative to the object base.
 */
void Gfx_EmitSmoke(struct game_object * o, int offset)
{
    if (snd_firecrackle_timeout == 0)
    {
        SoundInterface_PlaySfx_Pre(o, SFX_ATK_FIRE_CRACKLE);

        snd_firecrackle_timeout = (8 / V_MUL);
    }

    bool emit_smoke = false;

    if (o->id == OBJID_BOSS_TEST1)
    {
        // Increase density
        if (!((uint16_t)(system_frames_elapsed) & ANI_INTERVAL_4))
        {
            emit_smoke = true;
        }
    }
    else
    {
        if (!((uint16_t)(system_frames_elapsed) & FX_SMOKE_INTERVAL))
        {
            emit_smoke = true;
        }
    }

    if (emit_smoke)
    {
        int16_t temp_x = o->pos.x.lh.h + (o->w >> 1) - 8 + (((int)Math_GetRandom_u16() - 16384) % 16);
        int16_t temp_y = o->pos.y.lh.h - offset;

        if ((obj_first_available != 0xffff) && (gfx_smoke_queue_count < GFX_SMOKE_QUEUE_MAX_COUNT))
        {
            uint8_t queue_last = gfx_smoke_queue_first + gfx_smoke_queue_count;
            if (queue_last >= GFX_SMOKE_QUEUE_MAX_COUNT)
            {
                queue_last -= GFX_SMOKE_QUEUE_MAX_COUNT;
            }

            gfx_smoke_queue[queue_last].l = temp_x;
            gfx_smoke_queue[queue_last].h = temp_y;
            gfx_smoke_queue_count++;
        }
    }

    return;
}
