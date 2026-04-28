#include <stdint.h>

#include "vars.h"
#include "math_int.h"

/* 
    C integer version of atan2 adapted from 
    Discord: flipphone22/hibber22
    returns angle from 0-255
*/

uint8_t atan2_uint8(int16_t y, int16_t x)
{
    if (x == 0 && y == 0) {
        return 0;
    }
    int16_t abs_y = (y < 0) ? -y : y;
    uint8_t angle;
    if (x >= 0) {
        angle = 64 - (64 * (x - abs_y)) / (x + abs_y);
    } else {
        angle = 192 - (64 * (x + abs_y)) / (abs_y - x);
    }

    //angle /= 2;
    angle = (angle >> 1) + 128;

    return (y < 0) ? (256 - angle) : angle;
}

uint16_t rand_get16()
{
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
}

/*
    Seeds the random value generator LSFR with a 24-bit value (highest 8 bits dropped)
*/
void rand_seed(uint32_t s)
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