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
