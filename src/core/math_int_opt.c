#include <stdint.h>

#include "vars.h"
#include "math_int.h"

#if VBCC_ASM == 0
uint16_t Math_GetRandom_u16()
{
    uint8_t r0 = (uint8_t)rand_array[0];
    uint8_t r1 = (uint8_t)rand_array[1];
    uint8_t r2 = (uint8_t)rand_array[2];

    uint8_t c1 = (r0 & 0x80) ? 1 : 0;
    uint8_t r1_new = ((r0 << 1) ^ r1) & 0xff;

    uint8_t c2 = (r1_new & 0x80) ? 1 : 0;
    uint8_t a = ((r1_new << 1) | c1) & 0xff;

    uint8_t r2_new = a ^ r2;
    uint8_t r0_new = r2_new ^ r0;

    uint8_t a2 = r1_new;
    a2 = (a2 >> 1) | (c2 << 7);

    uint8_t r2_new2 = a2 ^ r2_new;
    uint8_t r1_new2 = r2_new2 ^ r1_new;

    rand_array[0] = (int8_t)r0_new;
    rand_array[1] = (int8_t)r1_new2;
    rand_array[2] = (int8_t)r2_new2;

    return (uint16_t)r0_new | ((uint16_t)r1_new2 << 8);
}
#endif
