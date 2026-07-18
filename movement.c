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

    int32_t delta_x = o->delta.x.a;
    int32_t delta_y = o->delta.y.a;
    int32_t pos_x = o->pos.x.a;
    int32_t pos_y = o->pos.y.a;

    // first test X axis movement
    int32_t temp_xl = pos_x + delta_x; // Must be done in 32-bit space
    
    uint16_t temp_x_pixel = (uint16_t)(temp_xl >> 16) + 1;
    uint16_t temp_y = (uint16_t)(pos_y >> 16) + 1;
    
    // Get position tile
    if (delta_x > 0)
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
        delta_x = 0;
    }

    // Additional check for screen bounds if it exists
    if (bg_scroll_x_bounds_min.full.high.a != -32768)
    {
        if (temp_x_pixel < bg_scroll_x_bounds_min.full.high.a)
        {
            delta_x = 0;
        }
        else if (temp_x_pixel > bg_scroll_x_bounds_max.full.high.a + 256)
        {
            delta_x = 0;
        }
    }
    
    // Now for the top edge Y axis moves
    uint32_t temp_yl = pos_y + delta_y;
    
    temp_x = (uint16_t)(pos_x >> 16) + 1;
    uint16_t temp_y_pixel = (uint16_t)(temp_yl >> 16) + 1;
    
    // Get position tile
    if (delta_y > 0)
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
        delta_y = 0;
        if ((delta_x | delta_y) == 0)
        {
            return 1;
        }
    }

    if (bg_scroll_y_bounds_min.full.high.a != -32768)
    {
        if (temp_y_pixel < bg_scroll_y_bounds_min.full.high.a)
        {
            delta_y = 0;
        }
        else if (temp_y_pixel > bg_scroll_y_bounds_max.full.high.a + 224)
        {
            delta_y = 0;
        }

        if ((delta_x | delta_y) == 0)
        {
            return 1;
        }
    }

    // Add the deltas here
    o->pos.x.a = pos_x + delta_x;
    o->pos.y.a = pos_y + delta_y;
    o->delta.x.a = delta_x;
    o->delta.y.a = delta_y;

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

            "\tphb\n"
            "\tphx\n"
            "\tplb\n"

            "\ttax\n" // Set up the pointer

            "\tlda !$0006,x\n"
            "\tclc\n"
            "\tadc !$0012,x\n"
            "\tsta !$0006,x\n"
            "\tlda !$0008,x\n"
            "\tadc !$0014,x\n"
            "\tsta !$0008,x\n"
            "\tclc\n"
            "\tadc !$0028,x\n"
            "\tsta !$002c,x\n"

            "\tlda !$000a,x\n"
            "\tclc\n"
            "\tadc !$0016,x\n"
            "\tsta !$000a,x\n"
            "\tlda !$000c,x\n"
            "\tadc !$0018,x\n"
            "\tsta !$000c,x\n"
            "\tclc\n"
            "\tadc !$002a,x\n"
            "\tsta !$002e,x\n"

            "\tplb\n"
            "\tplb\n"
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
            
            "\tphb\n"
            "\tphx\n"
            "\tplb\n"

            "\ttax\n" // Set up the pointer

            "\tlda !$0006,x\n"
            "\tclc\n"
            "\tadc !$0012,x\n"
            "\tsta !$0006,x\n"
            "\tlda !$0008,x\n"
            "\tadc !$0014,x\n"
            "\tsta !$0008,x\n"

            "\tlda !$000a,x\n"
            "\tclc\n"
            "\tadc !$0016,x\n"
            "\tsta !$000a,x\n"
            "\tlda !$000c,x\n"
            "\tadc !$0018,x\n"
            "\tsta !$000c,x\n"

            "\tplb\n"
            "\tplb\n"
            )   ;

    #else
        o->pos.x.a += o->delta.x.a;
        o->pos.y.a += o->delta.y.a;
    #endif

    return;
}
