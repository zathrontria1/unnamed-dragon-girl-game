#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "routines.h"
#include "obj.h"
#include "hittest.h"

#include "ani.h"

#include "map.h"
#include "spr.h"

#include "movement.h"

#include "math_int.h"

#include "snd.h"
#include "consts_snd.h"
#include "system.h"

#include "gfx.h"

uint16_t move(struct game_object * o)
{
    __asm("\tbit $4200\n");
    // one axis needs to be tested at a time
    // if a collision check fails on an axis,
    // that axis' delta will be zeroed out

    // first test X axis movement
    int32_t temp_xl = o->pos.x.a + o->delta.x.a; // Must be done in 32-bit space
    
    int16_t temp_x = (int16_t)(temp_xl >> 16) + 1;
    int16_t temp_y = o->pos.y.lh.h + 1;
    
    // Get position tile
    if (o->delta.x.a > 0)
    {
        temp_x += 13;
    }

    int16_t temp_y_2 = temp_y + 13; // Lower edge

    // Needed for blocker tests
    int16_t temp_x_pixel = temp_x;

    temp_x >>= 4;
    temp_y >>= 4;
    temp_y_2 >>= 4;

    // Top edge.
    uint16_t temp_screen_x;
    uint16_t temp_screen_y;

    uint16_t temp_start_x;

    uint16_t temp_test_failed = 0;

    temp_start_x = (temp_x & 0xf);
    temp_screen_x = temp_x >> 4;

    uint16_t temp_start_y = (temp_y & 0xf) << 4;
    temp_screen_y = temp_y >> 4;

    uint16_t temp_screen_offset;

    temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

    uint16_t q;
    q = temp_screen_offset + temp_start_x + temp_start_y;

    //const uint8_t * q;
    //q = map_current + 2 + temp_screen_offset + temp_start_x + temp_start_y;

    struct tile_xy t;
    t.x = temp_x;
    t.y = temp_y;

    if (map_collision_buf[q] < 128)
    //if ((map_lut_col[*q] < 128))
    //if ((map_lut_col[*q] < 128) || hit_test_blocker(t))
    {
        o->delta.x.a = 0;
        temp_test_failed = 1;
    }

    // Bottom edge
    if (temp_test_failed != 1)
    {
        temp_start_y = (temp_y_2 & 0xf) << 4;
        temp_screen_y = temp_y_2 >> 4;

        temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

        q = temp_screen_offset + temp_start_x + temp_start_y;
        //q = map_current + 2 + temp_screen_offset + temp_start_x + temp_start_y;

        t.y = temp_y_2;

        if (map_collision_buf[q] < 128)
        //if ((map_lut_col[*q] < 128))
        //if ((map_lut_col[*q] < 128) || hit_test_blocker(t))
        {
            o->delta.x.a = 0;
        }
    }

    // Additional check for screen bounds if it exists
    if (bg_scroll_x_bounds_min.full.high.a != -32768)
    {
        if (temp_x_pixel < bg_scroll_x_bounds_min.full.high.a)
        {
            o->delta.x.a = 0;
            temp_test_failed = 1;
        }
        else if (temp_x_pixel > bg_scroll_x_bounds_max.full.high.a + 256)
        {
            o->delta.x.a = 0;
            temp_test_failed = 1;
        }
    }
    
    // Now for the top edge Y axis moves
    int32_t temp_yl = o->pos.y.a + o->delta.y.a;
    
    temp_x = o->pos.x.lh.h + 1;
    temp_y = (int16_t)(temp_yl >> 16) + 1;
    
    // Get position tile
    if (o->delta.y.a > 0)
    {
        temp_y += 13;
    }

    int16_t temp_y_pixel = temp_y;
    
    int16_t temp_x_2 = temp_x + 13; // right edge

    temp_x >>= 4;
    temp_y >>= 4;
    temp_x_2 >>= 4;

    temp_test_failed = 0;

    // Left edge.
    temp_start_x = (temp_x & 0xf);
    temp_screen_x = temp_x >> 4;

    temp_start_y = (temp_y & 0xf) << 4;
    temp_screen_y = temp_y >> 4;

    temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

    q = temp_screen_offset + temp_start_x + temp_start_y;
    //q = map_current + 2 + temp_screen_offset + temp_start_x + temp_start_y;

    t.x = temp_x;
    t.y = temp_y;

    if (map_collision_buf[q] < 128)
    //if ((map_lut_col[*q] < 128))
    //if ((map_lut_col[*q] < 128) || hit_test_blocker(t))
    {
        o->delta.y.a = 0;
        if ((o->delta.x.a | o->delta.y.a) == 0)
        {
            return 1;
        }
        temp_test_failed = 1;
    }

    // Right edge
    if (temp_test_failed != 1)
    {
        temp_start_x = (temp_x_2 & 0xf);
        temp_screen_x = temp_x_2 >> 4;

        temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

        q = temp_screen_offset + temp_start_x + temp_start_y;
        //q = map_current + 2 + temp_screen_offset + temp_start_x + temp_start_y;

        t.x = temp_x_2;
        
        if (map_collision_buf[q] >= 128)
        //if ((map_lut_col[*q] >= 128))
        //if ((map_lut_col[*q] >= 128) || hit_test_blocker(t))
        {
            // One final check if a bounding box exists
            if (bg_scroll_y_bounds_min.full.high.a != -32768)
            {
                if (temp_y_pixel < bg_scroll_y_bounds_min.full.high.a)
                {
                    o->delta.y.a = 0;
                }
                else if (temp_y_pixel > bg_scroll_y_bounds_max.full.high.a + 224)
                {
                    o->delta.y.a = 0;
                }

                if ((o->delta.x.a | o->delta.y.a) == 0)
                {
                    return 1;
                }
            }

            o->pos.y.a += o->delta.y.a; // only change Y position if all Y tests pass
        }
        else
        {
            o->delta.y.a = 0;
            if ((o->delta.x.a | o->delta.y.a) == 0)
            {
                return 1;
            }
        }
    }

    // X must be done as the final calculation
    o->pos.x.a += o->delta.x.a; // Always saved

    // Update right and bottom edges
    o->r = o->pos.x.lh.h + o->w;
    o->b = o->pos.y.lh.h + o->h;

    return 0;
}

#if VBCC_ASM == 1
    NO_INLINE void move_nocol_fast(__reg("a/x") struct game_object * o)
#else
    inline void move_nocol_fast(struct game_object * o)
#endif
{
    // Move an object ignoring everything
    // Useful for light objects that do not need to test anything.
    // optionally also ignoring map edge

    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"
            "\ttax\n" // Set up the pointer

            "\tlda $7e0006,x\n"
            "\tclc\n"
            "\tadc $7e0012,x\n"
            "\tsta $7e0006,x\n"
            "\tlda $7e0008,x\n"
            "\tadc $7e0014,x\n"
            "\tsta $7e0008,x\n"
            "\tclc\n"
            "\tadc $7e0028,x\n"
            "\tsta $7e002c,x\n"

            "\tlda $7e000a,x\n"
            "\tclc\n"
            "\tadc $7e0016,x\n"
            "\tsta $7e000a,x\n"
            "\tlda $7e000c,x\n"
            "\tadc $7e0018,x\n"
            "\tsta $7e000c,x\n"
            "\tclc\n"
            "\tadc $7e002a,x\n"
            "\tsta $7e002e,x\n"
            )   ;

    #else
        o->pos.x.a += o->delta.x.a;
        o->pos.y.a += o->delta.y.a;

        // Update right and bottom edges
        o->r = o->pos.x.lh.h + o->w;
        o->b = o->pos.y.lh.h + o->h;
    #endif

    return;
}

#if VBCC_ASM == 1
    NO_INLINE void move_nocol_veryfast(__reg("a/x") struct game_object * o)
#else
    inline void move_nocol_veryfast(struct game_object * o)
#endif
{
    // Move an object ignoring everything
    // Useful for light objects that do not need to test anything.
    // optionally also ignoring map edge

    // This version will not update edges, so should be used for no-collision checking objects only

    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"
            "\ttax\n" // Set up the pointer

            "\tlda $7e0006,x\n"
            "\tclc\n"
            "\tadc $7e0012,x\n"
            "\tsta $7e0006,x\n"
            "\tlda $7e0008,x\n"
            "\tadc $7e0014,x\n"
            "\tsta $7e0008,x\n"

            "\tlda $7e000a,x\n"
            "\tclc\n"
            "\tadc $7e0016,x\n"
            "\tsta $7e000a,x\n"
            "\tlda $7e000c,x\n"
            "\tadc $7e0018,x\n"
            "\tsta $7e000c,x\n"
            )   ;

    #else
        o->pos.x.a += o->delta.x.a;
        o->pos.y.a += o->delta.y.a;
    #endif

    return;
}
