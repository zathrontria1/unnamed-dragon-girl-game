#include <stdint.h>

#include "vars.h"

#include "ani.h"
#include "spr.h"

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

            spr_queue_add_back(&temp, temp_tileattrib);
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
            temp_tilenum += o->ani.frame;
        }
    }
    else
    {
        // Now add the frame offset.
        // Still needed
        temp_tilenum += o->ani.frame;
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

uint16_t ani_getframe_fixed_fast(struct game_object * o)
{
    // Return offset to a fixed sprite tilenum based on given information
    // object ID and frame only
    // shorter version for light objects
    switch (o->id)
    {
        case OBJID_FX_SMOKE:
            return 6+(o->ani.frame << 1);
        case OBJID_FIREBALL:
            return 2+(o->ani.frame << 1);
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
    temp_tilenum += o->ani.frame;

    // Calculate the address
    return (uint8_t *)((uint32_t)&data_sprite_slime + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10));
}

uint8_t * ani_getframe_dynamic_slime(struct game_object * o)
{
    // use a virtual tilenum system before finalizing.
    uint16_t temp_tilenum = 0;

    switch (o->state)
    {
        case STATE_IDLE:
            break;
        case STATE_MOVE_WALK:
            temp_tilenum += 3;
            break;
        case STATE_MOVE_RUN:
            temp_tilenum += 3;
            break;
        case STATE_ATTACK_BASIC:
        case STATE_ATTACK_BASIC_MOVE:
            temp_tilenum += 18;
            break;
        case STATE_ATTACK_SPECIAL:
        case STATE_ATTACK_SPECIAL_MOVE:
            temp_tilenum += 24;
            break;
        case STATE_HURT_NORMAL:
            break;
        case STATE_HURT_NORMAL_MOVE:
            temp_tilenum += 3;
            break;
        case STATE_HURT_BURN:
            temp_tilenum += 9;
            break;
        case STATE_HURT_BURN_MOVE:
            temp_tilenum += 12;
            break;
        case STATE_DIE:
            temp_tilenum += 32;
            break;
        case STATE_SPAWNING:
            temp_tilenum += o->ani.frame;
            return (uint8_t *)((uint32_t)&data_sprite_spawn_placeholder + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10));
        default:
            break;
    }

    uint16_t temp_flip_x = 0;

    if (o->state != STATE_DIE)
    {
        if (o->state == STATE_IDLE)
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
                    temp_tilenum += 2;
                    temp_flip_x = 1;
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
                    temp_tilenum += 4;
                    temp_flip_x = 1;
                    break;
            }
        }
    }

    // Now add the frame offset.
    temp_tilenum += o->ani.frame;

    // Calculate the tilenum
    if (temp_flip_x)
    {
        // Also set the flip x flag
        return (uint8_t *)(((uint32_t)&data_sprite_slime + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10)) | 0x80000000); // set the negative flag
    }
    else
    {
        // Calculate the address
        return (uint8_t *)((uint32_t)&data_sprite_slime + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10));
    }
}
