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

/*
    Moves an object in the game world respecting all collision
*/
uint16_t ObjectSystem_Move(struct game_object * o)
{
    // one axis needs to be tested at a time
    // if a collision check fails on an axis,
    // that axis' delta will be zeroed out

    // first test X axis movement
    int32_t temp_xl = o->pos.x.a + o->delta.x.a; // Must be done in 32-bit space
    
    uint16_t temp_x_pixel = (uint16_t)(temp_xl >> 16) + 1;
    uint16_t temp_y = o->pos.y.lh.h + 1;
    
    // Get position tile
    if (o->delta.x.a > 0)
    {
        temp_x_pixel += 13;
    }

    uint16_t temp_y_2 = (temp_y + 13) >> 4; // Lower edge
    uint16_t temp_x = temp_x_pixel >> 4;
    temp_y >>= 4;

    // Top edge.
    uint16_t q = ((uint8_t)temp_y << map_extent_tiles_x_shiftcount) + temp_x;
    uint16_t q2 = ((uint8_t)temp_y_2 << map_extent_tiles_x_shiftcount) + temp_x;

    if ((map_collision_buf[q] < 128) || (map_collision_buf[q2] < 128))
    {
        o->delta.x.a = 0;
    }

    // Additional check for screen bounds if it exists
    if (bg_scroll_x_bounds_min.full.high.a != -32768)
    {
        if (temp_x_pixel < bg_scroll_x_bounds_min.full.high.a)
        {
            o->delta.x.a = 0;
        }
        else if (temp_x_pixel > bg_scroll_x_bounds_max.full.high.a + 256)
        {
            o->delta.x.a = 0;
        }
    }
    
    // Now for the top edge Y axis moves
    uint32_t temp_yl = o->pos.y.a + o->delta.y.a;
    
    temp_x = o->pos.x.lh.h + 1;
    uint16_t temp_y_pixel = (uint16_t)(temp_yl >> 16) + 1;
    
    // Get position tile
    if (o->delta.y.a > 0)
    {
        temp_y_pixel += 13;
    }

    temp_y = temp_y_pixel >> 4;
    uint16_t temp_x_2 = (temp_x + 13) >> 4; // right edge
    temp_x >>= 4;

    // Left edge.
    q = ((uint8_t)temp_y << map_extent_tiles_x_shiftcount) + temp_x;
    q2 = ((uint8_t)temp_y << map_extent_tiles_x_shiftcount) + temp_x_2;

    if (map_collision_buf[q] < 128 || map_collision_buf[q2] < 128)
    {
        o->delta.y.a = 0;
        if ((o->delta.x.a | o->delta.y.a) == 0)
        {
            return 1;
        }
    }

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

    // Add the deltas here
    o->pos.x.a += o->delta.x.a;
    o->pos.y.a += o->delta.y.a;

    // Update right and bottom edges
    o->r = o->pos.x.lh.h + o->w;
    o->b = o->pos.y.lh.h + o->h;

    return 0;
}

/*
    Move an object ignoring everything
    Useful for light objects that do not need to test anything.
    optionally also ignoring map edge
*/
#if VBCC_ASM == 1
    NO_INLINE void ObjectSystem_MoveWithoutCollision(__reg("a/x") struct game_object * o)
#else
    void ObjectSystem_MoveWithoutCollision(struct game_object * o)
#endif
{
    

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

/*
    Move an object ignoring everything
    Useful for light objects that do not need to test anything.
    optionally also ignoring map edge

    This version will not update edges, so should be used for no-collision checking objects only
*/
#if VBCC_ASM == 1
    NO_INLINE void ObjectSystem_MoveWithoutCollision_Fast(__reg("a/x") struct game_object * o)
#else
    void ObjectSystem_MoveWithoutCollision_Fast(struct game_object * o)
#endif
{
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
