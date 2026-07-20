#include <stdint.h>
#include <stdbool.h>
#include "consts.h"

extern int8_t rand_array[3];
extern uint16_t rand_seeded;

uint8_t Math_GetAtan2_u8(int16_t y, int16_t x);

#if VBCC_ASM == 1
NO_INLINE uint16_t Math_GetRandom_u16();
#else
uint16_t Math_GetRandom_u16();
#endif

void Math_SeedRandom(uint32_t s);
uint32_t Math_GetDistanceSquared(int16_t x, int16_t y);

#define Math_Sin(angle) (data_sine_1[(uint8_t)(angle)])
#define Math_Cos(angle) (data_sine_1[(uint8_t)((angle) + 64)])
