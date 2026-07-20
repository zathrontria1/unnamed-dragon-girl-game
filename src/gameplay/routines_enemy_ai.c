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
bool Routines_Enemy_Ai_Process(struct game_object * o, uint32_t dist, int16_t x, int16_t y, uint32_t dist_min, bool allow_flipflop, uint16_t attack_delay)
{
    struct game_data_npc * npc = &o->struct_data.npc_data;
    bool temp_invalidate_animation_frame = false;
    uint16_t temp_interrupted = 0;
    uint16_t ai_state = npc->ai_state;

    // Do some AI interruption given some conditions
    if (ai_state != AI_STATE_ATTACK)
    {
        if (((dist <= dist_min) && (ai_state == AI_STATE_MOVE_TOWARDS)) || (npc->status == STATUS_BURNING))
        {
            // If AI gets too close to the player while moving towards
            // OR if the AI is on fire, HOWEVER
            // Do not let the AI get interrupted if mid-attack
            npc->ai_timer = 0;
            temp_interrupted = 1;
        }
        else if (!allow_flipflop)
        {
            if ((dist < dist_min) && (ai_state == AI_STATE_MOVE_TOWARDS))
            {
                // If AI gets too close to the player while moving towards and flipflopping is disallowed
                npc->ai_timer = 0;
                temp_interrupted = 1;
            }
            else if ((dist > dist_min) && (ai_state == AI_STATE_MOVE_AWAY))
            {
                // If AI gets too far from the player while moving away and flipflopping is disallowed
                npc->ai_timer = 0;
            }
        }
    }

    if (npc->ai_timer == 0) // Only process AI state changes when this timer hits 0
    {
        uint8_t temp_rand = (uint8_t)Math_GetRandom_u16();
        
        // If the AI is idling, or gets interrupted by the player while not attacking...
        if (ai_state == AI_STATE_IDLE || (temp_interrupted && (ai_state != AI_STATE_ATTACK)))
        {
            // Object has finished idling, time to move once more.
            // Check if a minimum distance is met or not
            if ((dist > dist_min) && (npc->status != STATUS_BURNING))
            {
                // Enemy is far away from player, and should move closer if allowed
                uint8_t temp_angle = Math_GetAtan2_u8(y, x) + (temp_rand & 0x0f) - 8;

                o->angle = temp_angle;

                o->delta.x.a = Math_Cos(temp_angle) * V_MUL;
                o->delta.y.a = Math_Sin(temp_angle) * V_MUL;

                npc->ai_state = AI_STATE_MOVE_TOWARDS;
                if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
                {
                    o->state = STATE_HURT_BURN_MOVE;
                }
                else
                {
                    o->state = STATE_MOVE_WALK;
                }

                o->facing = Routines_Enemy_GetFacing(o);

                temp_invalidate_animation_frame = true;
            }
            else if (ai_state != AI_STATE_MOVE_AWAY) // don't recalc for anything already moving away.
            {
                // Enemy is close to player, and should move away
                uint8_t temp_angle = (Math_GetAtan2_u8(y, x)) + (temp_rand & 0x0f) - 136;

                o->angle = temp_angle;

                o->delta.x.a = Math_Cos(temp_angle) * V_MUL;
                o->delta.y.a = Math_Sin(temp_angle) * V_MUL;

                npc->ai_state = AI_STATE_MOVE_AWAY;

                if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
                {
                    o->state = STATE_HURT_BURN_MOVE;
                }
                else
                {
                    o->state = STATE_MOVE_WALK;
                }

                o->facing = Routines_Enemy_GetFacing(o);

                temp_invalidate_animation_frame = true;
            }

            npc->ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;
        }
        else if ((ai_state == AI_STATE_MOVE_TOWARDS) || (ai_state == AI_STATE_MOVE_AWAY))
        {
            // Object has finished its movement time and should decide on a possible action.
            o->delta.x.a = 0;
            o->delta.y.a = 0;

            if (ai_state == AI_STATE_MOVE_AWAY)
            {
                o->angle = (o->angle) + 128;
                o->facing = Routines_Enemy_GetFacing(o);
            }

            // Now to decide if the AI is going to attack or not.
            if (temp_rand >= 128)
            {
                // around half chance of performing an attack.
                // During this situation, check the previous state
                // to pick the correct attack for the range.

                if (ai_state == AI_STATE_MOVE_TOWARDS)
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
                npc->ai_state = AI_STATE_ATTACK;
                
                npc->ai_timer = attack_delay;

                npc->ai_makeattack = 1;
            }
            else
            {
                // Not attacking. Fix the state so the animation makes sense.
                npc->ai_state = AI_STATE_IDLE;

                if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
                {
                    o->state = STATE_HURT_BURN;
                }
                else
                {
                    o->state = STATE_IDLE;
                }

                npc->ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;
            } 

            temp_invalidate_animation_frame = true;
        }
        else if (ai_state == AI_STATE_ATTACK)
        {
            // Reset the AI to idle
            npc->ai_state = AI_STATE_IDLE;

            if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
            {
                o->state = STATE_HURT_BURN;
            }
            else
            {
                o->state = STATE_IDLE;
            }

            npc->ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;

            temp_invalidate_animation_frame = true;
        }
    }

    npc->ai_timer--;

    return temp_invalidate_animation_frame;
}

/*
    Simple function that only reduces AI delay timer
*/
void Routines_Enemy_Ai_Idle(struct game_object * o)
{
    o->struct_data.npc_data.ai_timer--;

    return;
}
