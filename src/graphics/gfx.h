#ifndef GFX_H
#define GFX_H

#include "consts.h"

extern bool gfx_enable_hitblur;
extern bool gfx_enable_heatwave;

extern uint16_t gfx_mosaic_layers;
extern int16_t gfx_mosaic_intensity;
extern int16_t gfx_mosaic_change;

extern int16_t gfx_cmath_change;

extern int16_t gfx_cmath_r;
extern int16_t gfx_cmath_g;
extern int16_t gfx_cmath_b;

#define GFX_SMOKE_QUEUE_MAX_COUNT OBJ_GENERAL_MAX_COUNT

struct game_object;

void Gfx_ProcessMosaic();
void Gfx_ProcessColorMath();
void Gfx_ProcessSmoke();
void Gfx_ResetSmoke();
void Gfx_SetColorMath(int16_t r, int16_t g, int16_t b, bool gradient);

void Gfx_EmitSmoke(struct game_object * o, int offset);

#endif // GFX_H

