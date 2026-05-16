FORCE_INLINE uint8_t Math_GetAtan2_u8(int16_t y, int16_t x);

#if VBCC_ASM == 1
NO_INLINE uint16_t Math_GetRandom_u16();
#else
uint16_t Math_GetRandom_u16();
#endif

void Math_SeedRandom(uint32_t s);

FORCE_INLINE uint32_t Math_GetDistanceSquared(int16_t x, int16_t y);
