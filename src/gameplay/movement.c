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

#define CORNER_NUDGE_THRESHOLD 5

static bool ObjectSystem_Move_TryNudgeCornerX(struct game_object * o, uint16_t q, uint16_t q2, uint16_t temp_x, uint16_t shiftcount)
{
    uint8_t c1 = map_collision_buf[q];
    uint8_t c2 = map_collision_buf[q2];

    if ((c1 < 128) != (c2 < 128))
    {
        uint16_t orig_y = o->pos.y.lh.h + 1;
        if (c1 < 128)
        {
            // Top corner blocked, bottom corner open -> nudge 1px DOWN (+Y)
            uint16_t overlap = 16 - (orig_y & 0x000f);
            if (overlap <= CORNER_NUDGE_THRESHOLD)
            {
                o->pos.y.lh.h += 1;
                o->b = o->pos.y.lh.h + o->h;
                uint16_t new_temp_y = (o->pos.y.lh.h + 1) >> 4;
                uint16_t new_shift_y;
                if (shiftcount == 6) {
                    new_shift_y = new_temp_y << 6;
                } else if (shiftcount == 5) {
                    new_shift_y = new_temp_y << 5;
                } else if (shiftcount == 7) {
                    new_shift_y = new_temp_y << 7;
                } else if (shiftcount == 4) {
                    new_shift_y = new_temp_y << 4;
                } else {
                    new_shift_y = new_temp_y << shiftcount;
                }
                if (map_collision_buf[new_shift_y + temp_x] >= 128)
                {
                    return true;
                }
            }
        }
        else
        {
            // Bottom corner blocked, top corner open -> nudge 1px UP (-Y)
            uint16_t overlap = ((orig_y + 13) & 0x000f) + 1;
            if (overlap <= CORNER_NUDGE_THRESHOLD)
            {
                o->pos.y.lh.h -= 1;
                o->b = o->pos.y.lh.h + o->h;
                uint16_t new_temp_y2 = (o->pos.y.lh.h + 1 + 13) >> 4;
                uint16_t new_shift_y2;
                if (shiftcount == 6) {
                    new_shift_y2 = new_temp_y2 << 6;
                } else if (shiftcount == 5) {
                    new_shift_y2 = new_temp_y2 << 5;
                } else if (shiftcount == 7) {
                    new_shift_y2 = new_temp_y2 << 7;
                } else if (shiftcount == 4) {
                    new_shift_y2 = new_temp_y2 << 4;
                } else {
                    new_shift_y2 = new_temp_y2 << shiftcount;
                }
                if (map_collision_buf[new_shift_y2 + temp_x] >= 128)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

static bool ObjectSystem_Move_TryNudgeCornerY(struct game_object * o, uint16_t q, uint16_t q2, uint16_t shift_temp_y)
{
    uint8_t c1 = map_collision_buf[q];
    uint8_t c2 = map_collision_buf[q2];

    if ((c1 < 128) != (c2 < 128))
    {
        uint16_t orig_x = o->pos.x.lh.h + 1;
        if (c1 < 128)
        {
            // Left corner blocked, right corner open -> nudge 1px RIGHT (+X)
            uint16_t overlap = 16 - (orig_x & 0x000f);
            if (overlap <= CORNER_NUDGE_THRESHOLD)
            {
                o->pos.x.lh.h += 1;
                o->r = o->pos.x.lh.h + o->w;
                uint16_t new_temp_x = (o->pos.x.lh.h + 1) >> 4;
                if (map_collision_buf[shift_temp_y + new_temp_x] >= 128)
                {
                    return true;
                }
            }
        }
        else
        {
            // Right corner blocked, left corner open -> nudge 1px LEFT (-X)
            uint16_t overlap = ((orig_x + 13) & 0x000f) + 1;
            if (overlap <= CORNER_NUDGE_THRESHOLD)
            {
                o->pos.x.lh.h -= 1;
                o->r = o->pos.x.lh.h + o->w;
                uint16_t new_temp_x2 = (o->pos.x.lh.h + 1 + 13) >> 4;
                if (map_collision_buf[shift_temp_y + new_temp_x2] >= 128)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * @brief Moves an object according to its delta velocity while testing map metatile collisions and camera bounds.
 * 
 * @param o Pointer to the target game object.
 * @return Collision flag bitmask.
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
            if (!ObjectSystem_Move_TryNudgeCornerX(o, q, q2, temp_x, shiftcount))
            {
                delta_x = 0;
                if (o->delta.x.a > 0)
                {
                    temp_xl.lh.h = (temp_x << 4) - 15;
                }
                else
                {
                    temp_xl.lh.h = ((temp_x + 1) << 4) - 1;
                }
                temp_xl.lh.l = 0;
                o->pos.x.a = temp_xl.a;
                o->r = temp_xl.lh.h + o->w;
            }
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

        if ((map_collision_buf[q] < 128) || (map_collision_buf[q2] < 128))
        {
            if (!ObjectSystem_Move_TryNudgeCornerY(o, q, q2, shift_temp_y))
            {
                delta_y = 0;
                if (o->delta.y.a > 0)
                {
                    temp_yl.lh.h = (temp_y << 4) - 15;
                }
                else
                {
                    temp_yl.lh.h = ((temp_y + 1) << 4) - 1;
                }
                temp_yl.lh.l = 0;
                o->pos.y.a = temp_yl.a;
                o->b = temp_yl.lh.h + o->h;
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

/**
 * @brief Moves an object by adding deltas to its position and updating bounding box right/bottom edges, bypassing collisions.
 * 
 * @param o [a/x] Pointer to the target game object.
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

/**
 * @brief High-speed movement routine for purely visual objects/particles that updates positions without recomputing bounding box edges.
 * 
 * @param o [a/x] Pointer to the target game object.
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
