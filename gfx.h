extern uint16_t gfx_mosaic_layers;
extern int16_t gfx_mosaic_intensity;
extern int16_t gfx_mosaic_change;

extern int16_t gfx_cmath_change;

extern int16_t gfx_cmath_r;
extern int16_t gfx_cmath_g;
extern int16_t gfx_cmath_b;

void gfx_process_mosaic();
void gfx_process_screen_cmath();
FORCE_INLINE void gfx_cmath_set(int16_t r, int16_t g, int16_t b);
