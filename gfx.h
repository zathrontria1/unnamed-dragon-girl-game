extern uint16_t gfx_mosaic_layers;
extern int16_t gfx_mosaic_intensity;
extern int16_t gfx_mosaic_change;

extern int16_t gfx_cmath_change;

extern int16_t gfx_cmath_r;
extern int16_t gfx_cmath_g;
extern int16_t gfx_cmath_b;

void Gfx_ProcessMosaic();
void Gfx_ProcessColorMath();
void Gfx_SetColorMath(int16_t r, int16_t g, int16_t b);

void Gfx_EmitSmoke(struct game_object * o);
