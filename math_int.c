#include <stdint.h>
#include <stdbool.h>

#include "vars.h"
#include "math_int.h"

// Random numbers
ZP int8_t rand_array[3];
uint16_t rand_seeded;

/* 
    C integer version of atan2 adapted from 
    Discord: flipphone22/hibber22
    returns angle from 0-255
*/

FORCE_INLINE uint8_t Math_GetAtan2_u8(int16_t y, int16_t x)
{
    if (x == 0 && y == 0) {
        return 0;
    }
    int16_t abs_y = (y < 0) ? -y : y;
    uint8_t angle;
    if (x >= 0) {
        angle = ((64 - (64 * (x - abs_y)) / (x + abs_y)) >> 1) + 128;
    } else {
        angle = ((192 - (64 * (x + abs_y)) / (abs_y - x)) >> 1) + 128;
    }

    return (y < 0) ? (256 - angle) : angle;
}

#if VBCC_ASM == 1
NO_INLINE uint16_t Math_GetRandom_u16()
#else
uint16_t Math_GetRandom_u16()
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\ta8\n"
            "\tsep #$20\n"
            "\tlda _rand_array\n"           // Operation 7 (with carry clear).
            "\tasl\n"
            "\teor _rand_array+1\n"
            "\tsta _rand_array+1\n"
            "\trol\n"             // Operation 9.
            "\teor _rand_array+2\n"
            "\tsta _rand_array+2\n"
            "\teor _rand_array\n"           // Operation 5.
            "\tsta _rand_array\n"
            "\tlda _rand_array+1\n"           // Operation 15.
            "\tror\n"
            "\teor _rand_array+2\n"
            "\tsta _rand_array+2\n"
            "\teor _rand_array+1\n"           // Operation 6.
            "\tsta _rand_array+1\n"
            "\ta16\n"
            "\trep #$30\n"
            "\tlda _rand_array\n"
            "\trtl\n"
        );
    #else
        uint8_t temp_carry = 0;

        rand_array[1] = (rand_array[0] << 1) ^ rand_array[1];

        if (rand_array[1] < 0)
        {
            rand_array[2] = ((rand_array[1] << 1) ^ rand_array[2]) | 0x01;
            temp_carry = 1;
        }
        else
        {
            rand_array[2] = (rand_array[1] << 1) ^ rand_array[2];
        }

        rand_array[0] = (rand_array[2] << 1) ^ rand_array[0];

        if (temp_carry)
        {
            rand_array[2] = ((rand_array[1] >> 1) | 0x80) ^ rand_array[2];
        }
        else
        {
            rand_array[2] = (rand_array[1] >> 1) ^ rand_array[2];
        }

        rand_array[1] = rand_array[2] ^ rand_array[1];

        uint16_t * val = (uint16_t *) (&rand_array[0]);

        return *val;
    #endif
    
    return 0;
}

/*
    Seeds the random value generator LSFR with a 24-bit value (highest 8 bits dropped)
*/
void Math_SeedRandom(uint32_t s)
{
    if (s == 0)
    {
        // LSFR cannot be 0
        // Treat it as 1 and unseeded (so can be subject to reseed)
        s = 1;
        rand_seeded = 0;
    }
    else
    {
        rand_seeded = 1;
    }

    rand_array[0] = (uint8_t)s;
    rand_array[1] = (uint8_t)(s >> 8);
    rand_array[2] = (uint8_t)(s >> 16);

    return;
}

/*
    Returns the squared distance (avoid a square root)

    Limited to max 320 on either axis to prevent the LUT from going too large.
*/
FORCE_INLINE uint32_t Math_GetDistanceSquared(int16_t x, int16_t y)
{
    // It's pretty fast already. Don't inline this.
    // c^2 = a^2 + b^2
    uint16_t abs_y = (y < 0) ? -y : y;
    uint16_t abs_x = (x < 0) ? -x : x;

    if (abs_x > 320)
    {
        abs_x = 320;
    }

    if (abs_y > 320)
    {
        abs_y = 320;
    }

    return (data_pow_2[abs_x] + data_pow_2[abs_y]);
}
