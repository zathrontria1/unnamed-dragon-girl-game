#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "routines.h"
#include "routines_player.h"
#include "routines_enemy.h"
#include "routines_enemy_ai.h"
#include "obj.h"
#include "hittest.h"

#include "ani.h"

#include "map.h"
#include "spr.h"

#include "math_int.h"

#include "snd.h"
#include "consts_snd.h"
#include "system.h"

#include "gfx.h"

/*
    Gameplay AI for non-player entities
*/
uint16_t ai_run(struct game_object * o, uint32_t dist, int16_t x, int16_t y, uint32_t dist_min, uint32_t dist_max)
{
    uint16_t temp_invalidate_animation_frame = 0;
    uint16_t temp_interrupted = 0;

    // Do some AI interruption given some conditions
    if (
        (((dist <= dist_min) && o->struct_data.npc_data.ai_state != AI_STATE_IDLE) ||
        (o->struct_data.npc_data.status == STATUS_BURNING)) && (o->struct_data.npc_data.ai_state != AI_STATE_ATTACK)
        )
    {
        // If AI gets too close to the player AND the AI isn't idling (i.e. moving)
        // OR if the AI is on fire, HOWEVER
        // Do not let the AI get interrupted if mid-attack
        o->struct_data.npc_data.ai_timer = 0;
        temp_interrupted = 1;
    }

    if (o->struct_data.npc_data.ai_timer == 0) // Only process AI state changes when this timer hits 0
    {
        uint8_t temp_rand = (uint8_t)Math_GetRandom_u16();
        
        // If the AI is idling, attacked the player, or gets interrupted by the player while not attacking...
        if (o->struct_data.npc_data.ai_state == AI_STATE_IDLE || (temp_interrupted && (o->struct_data.npc_data.ai_state != AI_STATE_ATTACK)))
        {
            // Object has finished idling, time to move once more.
            // Check if a minimum distance is met or not
            if ((dist > dist_min) && (o->struct_data.npc_data.status != STATUS_BURNING))
            {
                // Enemy is far away from player
                uint8_t temp_angle = Math_GetAtan2_u8(y, x) + (temp_rand & 0x0f) - 8;

                o->angle = temp_angle;

                o->delta.x.a = data_cosine_1[temp_angle] * V_MUL;
                o->delta.y.a = data_sine_1[temp_angle] * V_MUL;

                o->struct_data.npc_data.ai_state = AI_STATE_MOVE_TOWARDS;
                if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
                {
                    o->state = STATE_HURT_BURN_MOVE;
                }
                else
                {
                    o->state = STATE_MOVE_WALK;
                }

                o->facing = ai_get_facing(o);

                temp_invalidate_animation_frame = 1;
            }
            else if (o->struct_data.npc_data.ai_state != AI_STATE_MOVE_AWAY) // don't recalc for anything already moving away.
            {
                // Enemy is close to player
                uint8_t temp_angle = (Math_GetAtan2_u8(y, x)) + (temp_rand & 0x0f) - 136;

                o->angle = temp_angle;

                o->delta.x.a = data_cosine_1[temp_angle] * V_MUL;
                o->delta.y.a = data_sine_1[temp_angle] * V_MUL;

                o->struct_data.npc_data.ai_state = AI_STATE_MOVE_AWAY;

                if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
                {
                    o->state = STATE_HURT_BURN_MOVE;
                }
                else
                {
                    o->state = STATE_MOVE_WALK;
                }

                o->facing = ai_get_facing(o);

                temp_invalidate_animation_frame = 1;
            }

            o->struct_data.npc_data.ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;
        }
        else if ((o->struct_data.npc_data.ai_state == AI_STATE_MOVE_TOWARDS) || (o->struct_data.npc_data.ai_state == AI_STATE_MOVE_AWAY))
        {
            // Object has finished its movement time and should decide on a possible action.
            o->delta.x.a = 0;
            o->delta.y.a = 0;

            if (o->struct_data.npc_data.ai_state == AI_STATE_MOVE_AWAY)
            {
                o->angle = (o->angle) + 128;
                o->facing = ai_get_facing(o);
            }

            // Now to decide if the AI is going to attack or not.
            if (temp_rand >= 128)
            {
                // around half chance of performing an attack.
                // During this situation, check the previous state
                // to pick the correct attack for the range.

                if (o->struct_data.npc_data.ai_state == AI_STATE_MOVE_TOWARDS)
                {
                    if (dist <= dist_min)
                    {
                        o->state = STATE_ATTACK_BASIC;
                    }
                    else
                    {
                        o->state = STATE_ATTACK_SPECIAL; 
                    }
                }
                else
                {
                    o->state = STATE_ATTACK_SPECIAL; 
                }

                // For now, duplicate of the other case.
                o->struct_data.npc_data.ai_state = AI_STATE_ATTACK;
                
                o->struct_data.npc_data.ai_timer = (15) / V_MUL;

                o->struct_data.npc_data.ai_makeattack = 1;
            }
            else
            {
                // Not attacking. Fix the state so the animation makes sense.
                o->struct_data.npc_data.ai_state = AI_STATE_IDLE;

                if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
                {
                    o->state = STATE_HURT_BURN;
                }
                else
                {
                    o->state = STATE_IDLE;
                }

                o->struct_data.npc_data.ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;
            } 

            temp_invalidate_animation_frame = 1;
        }
        else if (o->struct_data.npc_data.ai_state == AI_STATE_ATTACK)
        {
            // Reset the AI to idle
            o->struct_data.npc_data.ai_state = AI_STATE_IDLE;

            if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
            {
                o->state = STATE_HURT_BURN;
            }
            else
            {
                o->state = STATE_IDLE;
            }

            o->struct_data.npc_data.ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;

            temp_invalidate_animation_frame = 1;
        }
    }

    ai_idle(o); // tick down the timer

    return temp_invalidate_animation_frame;
}

FORCE_INLINE void ai_idle(struct game_object * o)
{
    o->struct_data.npc_data.ai_timer--;

    return;
}

FORCE_INLINE uint16_t ai_get_facing(struct game_object * o)
{
    // Adjust the facing based on angle.
    if (o->angle < 32)
    {
        return FACING_RIGHT;
    }
    else if (o->angle < 96)
    {
        return FACING_DOWN;
    }
    else if (o->angle < 160)
    {
        return FACING_LEFT;
    }
    else if (o->angle < 224)
    {
        return FACING_UP;
    }
    else
    {
        return FACING_RIGHT;
    }
}
