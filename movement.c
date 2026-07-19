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
    int32_t delta_x = o->delta.x.a;
    int32_t delta_y = o->delta.y.a;

    bool has_x = (delta_x != 0);
    bool has_y = (delta_y != 0);

    union pos_32 temp_xl;
    union pos_32 temp_yl;

    if (has_x)
    {
        temp_xl.a = o->pos.x.a + delta_x;
        
        uint16_t temp_x_pixel = temp_xl.lh.h + 1;
        uint16_t temp_y = o->pos.y.lh.h + 1;
        
        if (delta_x > 0)
        {
            temp_x_pixel += 13;
        }

        uint16_t temp_y_2 = (temp_y + 13) >> 4; // Lower edge
        uint16_t temp_x = temp_x_pixel >> 4;
        temp_y >>= 4;

        uint16_t shiftcount = map_extent_tiles_x_shiftcount;
        uint16_t shift_temp_y, shift_temp_y2;
        if (shiftcount == 6) {
            shift_temp_y = temp_y << 6;
            shift_temp_y2 = temp_y_2 << 6;
        } else if (shiftcount == 5) {
            shift_temp_y = temp_y << 5;
            shift_temp_y2 = temp_y_2 << 5;
        } else if (shiftcount == 7) {
            shift_temp_y = temp_y << 7;
            shift_temp_y2 = temp_y_2 << 7;
        } else if (shiftcount == 4) {
            shift_temp_y = temp_y << 4;
            shift_temp_y2 = temp_y_2 << 4;
        } else {
            shift_temp_y = temp_y << shiftcount;
            shift_temp_y2 = temp_y_2 << shiftcount;
        }
        uint16_t q = shift_temp_y + temp_x;
        uint16_t q2 = shift_temp_y2 + temp_x;

        if ((map_collision_buf[q] < 128) || (map_collision_buf[q2] < 128))
        {
            delta_x = 0;
        }

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
    }

    if (has_y)
    {
        temp_yl.a = o->pos.y.a + delta_y;
        
        uint16_t temp_x = o->pos.x.lh.h + 1;
        uint16_t temp_y_pixel = temp_yl.lh.h + 1;
        
        if (delta_y > 0)
        {
            temp_y_pixel += 13;
        }

        uint16_t temp_y = temp_y_pixel >> 4;
        uint16_t temp_x_2 = (temp_x + 13) >> 4; // right edge
        temp_x >>= 4;

        uint16_t shiftcount = map_extent_tiles_x_shiftcount;
        uint16_t shift_temp_y;
        if (shiftcount == 6) {
            shift_temp_y = temp_y << 6;
        } else if (shiftcount == 5) {
            shift_temp_y = temp_y << 5;
        } else if (shiftcount == 7) {
            shift_temp_y = temp_y << 7;
        } else if (shiftcount == 4) {
            shift_temp_y = temp_y << 4;
        } else {
            shift_temp_y = temp_y << shiftcount;
        }
        uint16_t q = shift_temp_y + temp_x;
        uint16_t q2 = shift_temp_y + temp_x_2;

        if (map_collision_buf[q] < 128 || map_collision_buf[q2] < 128)
        {
            delta_y = 0;
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
        }
    }

    if (has_x)
    {
        if (delta_x == 0)
        {
            o->delta.x.a = 0;
        }
        else
        {
            o->pos.x.a = temp_xl.a;
            o->r = temp_xl.lh.h + o->w;
        }
    }

    if (has_y)
    {
        if (delta_y == 0)
        {
            o->delta.y.a = 0;
        }
        else
        {
            o->pos.y.a = temp_yl.a;
            o->b = temp_yl.lh.h + o->h;
        }
    }

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
