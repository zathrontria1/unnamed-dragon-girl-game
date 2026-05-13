#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "ani.h"
#include "spr.h"

// Lookup tables for animations
/*
    For reference:

    #define STATE_IDLE 0

    #define STATE_MOVE_WALK 1 // Yes, there is overlap
    #define STATE_MOVE_RUN 2

    #define STATE_ATTACK_BASIC 3
    #define STATE_ATTACK_BASIC_MOVE 4

    #define STATE_ATTACK_SPECIAL 5
    #define STATE_ATTACK_SPECIAL_MOVE 6

    #define STATE_HURT_NORMAL 7
    #define STATE_HURT_NORMAL_MOVE 8
    #define STATE_HURT_NORMAL_MOVE_RUN 9

    #define STATE_HURT_BURN 10
    #define STATE_HURT_BURN_MOVE 11

    #define STATE_SPAWNING 12
    #define STATE_DIE 13

    #define FACING_DOWN 0
    #define FACING_UP 1
    #define FACING_RIGHT 2
    #define FACING_LEFT 3
*/

// With flipping
const uint16_t const_ani_lut_basic[56] = 
{
   0, 1, 2, 2,

   3, 5, 7, 7,
   3, 5, 7, 7,

   18, 20, 22, 22,
   18, 20, 22, 22,

   24, 26, 28, 28,
   24, 26, 28, 28,
    
   0, 1, 2, 2,
   3, 5, 7, 7,
   3, 5, 7, 7,

   9, 10, 11, 11,
   12, 14, 16, 16,

   0, 0, 0, 0,
   32, 32, 32, 32,
};

/*
    Animations item drop gravity, and draw a drop shadow if mid-air
*/
uint16_t ani_animate_drop_gravity(struct game_object * o)
{
    uint16_t grounded = 0;

    if (!((o->pos.z.a == 0) && (o->delta.z.a == 0)))
    {
        
        o->pos.z.a += o->delta.z.a;
        o->delta.z.a -= (V_GRAVITY >> 1);

        if (o->pos.z.a <= 0)
        {
            o->pos.z.a = 0;
            o->delta.z.a = 0;

            grounded = 1;
        }
    }

    if (o->pos.z.a != 0)
    {
        // also draw a shadow if relevant
        if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
        {
            struct game_object temp;
            temp.pos.x.a = o->pos.x.a;
            temp.pos.y.a = o->pos.y.a;
            temp.pos.z.a = 0;

            uint16_t temp_tileattrib;
            temp_tileattrib = 0x0e | PAL_FX_SHADOW << 9 | 2 << 12;

            SpriteEngine_AddToBackLayer(&temp, temp_tileattrib);
        }
    }

    return grounded;
}

uint8_t * ani_getframe_player(struct game_object * o)
{
    // Return player sprite address based on given information
    // State, facing
    // sprites are in the order of down, up, right, left
    // use a virtual tilenum system before finalizing.
    uint16_t temp_tilenum = 0;

    switch (o->state)
    {
        case STATE_IDLE:
            break;
        case STATE_MOVE_WALK:
            temp_tilenum += 4;
            break;
        case STATE_MOVE_RUN:
            temp_tilenum += 40;
            break;
        case STATE_ATTACK_BASIC:
            temp_tilenum += 24;
            break;
        case STATE_ATTACK_BASIC_MOVE:
            temp_tilenum += 32;
            break;
        case STATE_ATTACK_SPECIAL:
            temp_tilenum += 12;
            break;
        case STATE_ATTACK_SPECIAL_MOVE:
            temp_tilenum += 16;
            break;
        case STATE_HURT_NORMAL:
            temp_tilenum += 48;
            break;
        case STATE_HURT_NORMAL_MOVE:
            temp_tilenum += 52;
            break;
        case STATE_HURT_NORMAL_MOVE_RUN:
            temp_tilenum += 60;
            break;
        case STATE_HURT_BURN:
            temp_tilenum += 48;
            break;
        case STATE_HURT_BURN_MOVE:
            temp_tilenum += 52;
            break;
        case STATE_ICON_NORMAL:
            temp_tilenum += 68;
            break;
        case STATE_ICON_BLINK:
            temp_tilenum += 69;
            break;
        case STATE_ICON_HURT:
            temp_tilenum += 70;
            break;
        case STATE_ICON_SPECIAL:
            temp_tilenum += 71;
            break;
        case STATE_DIE:
            temp_tilenum += 72;
            break;
    }

    if (o->state != STATE_DIE)
    {
        if (o->state < STATE_ICON_NORMAL)
        {
            if (o->state == STATE_IDLE || o->state == STATE_ATTACK_SPECIAL || o->state == STATE_HURT_NORMAL)
            {
                switch (o->facing)
                {
                    case FACING_DOWN:
                        break;
                    case FACING_UP:
                        temp_tilenum += 1;
                        break;
                    case FACING_RIGHT:
                        temp_tilenum += 2;
                        break;
                    case FACING_LEFT:
                        temp_tilenum += 3;
                        break;
                }
            }
            else
            {
                switch (o->facing)
                {
                    case FACING_DOWN:
                        break;
                    case FACING_UP:
                        temp_tilenum += 2;
                        break;
                    case FACING_RIGHT:
                        temp_tilenum += 4;
                        break;
                    case FACING_LEFT:
                        temp_tilenum += 6;
                        break;
                }
            }

            // Now add the frame offset.
            temp_tilenum += o->struct_data.npc_data.ani.frame;
        }
    }
    else
    {
        // Now add the frame offset.
        // Still needed
        temp_tilenum += o->struct_data.npc_data.ani.frame;
    }
    
    
    // Calculate the address
    return (uint8_t *)&data_sprite_player + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10);
}

uint8_t * ani_getframe_dynamic(struct game_object * o)
{
    switch (o->id)
    {
        case OBJID_SLIME:
            return ani_getframe_dynamic_slime(o);
        default:
            return 0;
    }
}

uint8_t * ani_getframe_dynamic_stateless(struct game_object * o)
{
    switch (o->id)
    {
        case OBJID_BUBBLE_E:
            return ani_getframe_dynamic_bubble(o);
        default:
            return 0;
    }
}

// Return offset to a fixed sprite tilenum based on given information
// object ID and frame only
// shorter version for light objects
uint16_t ani_getframe_fixed_fast(struct game_object * o)
{
    switch (o->id)
    {
        case OBJID_FX_SMOKE:
            return 6+(o->struct_data.npc_data.ani.frame << 1);
        case OBJID_FIREBALL:
            return 2+(o->struct_data.npc_data.ani.frame << 1);
        case OBJID_SYS_IMPACT:
            return 10;
        case OBJID_SYS_TARGET:
            return 14;
        default:
            return 0;
    }
}

uint8_t * ani_getframe_dynamic_bubble(struct game_object * o)
{
    // use a virtual tilenum system before finalizing.
    uint16_t temp_tilenum = 30;

    // Now add the frame offset.
    temp_tilenum += o->struct_data.npc_data.ani.frame;

    // Calculate the address
    return (uint8_t *)((uint32_t)&data_sprite_slime + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10));
}

#if VBCC_ASM == 1
NO_INLINE uint8_t * ani_getframe_dynamic_slime(__reg("a/x") struct game_object * o)
#else
uint8_t * ani_getframe_dynamic_slime(struct game_object * o)
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\tphy\n"

            "\tpei (r2)\n"

            "\ttax\n"

            "\tlda $7e001e,x\n"
            "\tcmp #12\n"
            "\tbne .not_spawning\n"

            ".spawning:\n"
            // Frame Number
            // Current frame is byte 64
            "\tlda $7e0040,x\n" // current frame. now we have the tile num in virtual space
            "\ttay\n"
            "\tand #$0007\n"
            "\txba\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tsta r2\n"

            "\ttya\n"
            "\txba\n"
            "\tlsr\n"
            "\tand #$7c00\n"
            
            "\tadc r2\n"
            "\tadc #<_data_sprite_spawn_placeholder\n"
            "\tldx #^_data_sprite_spawn_placeholder\n"

            "\tbra .finish\n"

            ".not_spawning:\n"
            // (State * 4 + Facing + Frame Number)
            // state is byte 30, facing is byte 32, current frame is byte 64
            "\tlda $7e001e,x\n" // state
            "\tasl\n"
            "\tasl\n" // Carry is cleared here
            "\tadc $7e0020,x\n" // facing
            
            "\tasl\n" // Now we have the index to look into the lookup
            "\ttxy\n"
            "\ttax\n"
            "\tlda >_const_ani_lut_basic, x\n"

            // Transform this number
            // (temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10)
            "\ttyx\n"
            "\tadc $7e0040,x\n" // current frame. now we have the tile num in virtual space
            "\ttay\n"
            "\tand #$0007\n"
            "\txba\n"
            "\tlsr\n"
            "\tlsr\n"
            "\tsta r2\n"

            "\ttya\n"
            "\txba\n"
            "\tlsr\n"
            "\tand #$7c00\n"
            "\tadc r2\n"
            "\tadc #<_data_sprite_slime\n" 
            "\ttay\n"

            // Test for sign flip
            "\tlda $7e001e,x\n" // state
            "\tcmp #13\n"
            "\tbeq .no_flip\n"
            "\tlda $7e0020,x\n" // facing
            "\tcmp #3\n"
            "\tbne .no_flip\n"
            ".flip_x:\n"
                "\tlda #$8000\n"
                "\tora #^_data_sprite_slime\n"
                "\ttax\n"
                "\tbra .finalize\n"
            ".no_flip:\n"
                "\tldx #^_data_sprite_slime\n"
            ".finalize:\n"
            "\ttya\n"

            ".finish:\n"
            "\tply\n"
            "\tsty r2\n"

            "\tply\n"

            "\trtl\n"
        );
    #else
        // use a virtual tilenum system before finalizing.
        uint16_t temp_tilenum = o->struct_data.npc_data.ani.frame; // add the frame offset.

        if (o->state == STATE_SPAWNING)
        {
            return (uint8_t *)((uint32_t)&data_sprite_spawn_placeholder + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10));
        }
        else
        {
            temp_tilenum += const_ani_lut_basic[(o->state << 2) + o->facing];

            if ((o->facing == FACING_LEFT) && (o->state != STATE_DIE))
            {
                return (uint8_t *)(((uint32_t)&data_sprite_slime + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10)) | 0x80000000); // set the negative flag
            }
            else
            {
                // Calculate the address
                return (uint8_t *)((uint32_t)&data_sprite_slime + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10));
            }
        }
    #endif

    return 0;
}
