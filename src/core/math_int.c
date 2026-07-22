#include <stdint.h>
#include <stdbool.h>

#include "vars.h"
#include "math_int.h"

/**
 * @brief LSFR random generator state buffer.
 */
int8_t rand_array[3];

/**
 * @brief Flag indicating if the random number generator has been seeded.
 */
uint16_t rand_seeded;

/**
 * @brief Calculates the 8-bit integer approximation of atan2.
 * 
 * Adapted from Discord user: flipphone22/hibber22.
 * Maps the result angle to a range of 0 to 255.
 * 
 * @param y The vertical coordinate offset.
 * @param x The horizontal coordinate offset.
 * @return The resulting angle in the range [0, 255].
 */
uint8_t Math_GetAtan2_u8(int16_t y, int16_t x)
{
    if (y == -32768)
    {
        y = -32767; // INT16_MIN isn't defined (yet)?
    }
    if (x == -32768)
    {
        x = -32767;
    }

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
    #endif
    
    return 0;
}

/**
 * @brief Seeds the pseudo-random number generator with a 24-bit seed.
 * 
 * @param s A 32-bit seed value (highest 8 bits are discarded).
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

/**
 * @brief Calculates the squared distance between two coordinate offsets.
 * 
 * Avoids slow square root operations. The offsets on either axis are clamped 
 * to 320 to avoid exceeding the bounds of the power-of-two lookup table.
 * 
 * @param x The horizontal distance offset.
 * @param y The vertical distance offset.
 * @return The squared distance (x^2 + y^2).
 */
uint32_t Math_GetDistanceSquared(int16_t x, int16_t y)
{
    if (x == -32768)
    {
        x = -32767; // INT16_MIN isn't defined (yet)?
    }
    if (y == -32768)
    {
        y = -32767;

    }
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
