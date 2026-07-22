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

static bool Routines_Enemy_Ai_IsTileInBounds(int16_t x, int16_t y)
{
    return x >= 0 &&
           y >= 0 &&
           x < (int16_t)map_extent_tiles_x &&
           y < (int16_t)map_extent_tiles_y;
}

/*
    Gameplay AI for non-player entities
*/

/*
    Metatile Bresenham tile raycast (returns true if path is clear of solid tiles)
*/
static bool Routines_Enemy_Ai_CheckRawTileLOS(int16_t ex, int16_t ey, int16_t px, int16_t py)
{
    int16_t dx = px - ex;
    int16_t dy = py - ey;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    int16_t sx = (ex < px) ? 1 : -1;
    int16_t sy = (ey < py) ? 1 : -1;
    int16_t err = dx - dy;

    uint16_t shiftcount = map_extent_tiles_x_shiftcount;

    int16_t curr_x = ex;
    int16_t curr_y = ey;

    if (!Routines_Enemy_Ai_IsTileInBounds(curr_x, curr_y))
    {
        return false; // Out of bounds
    }

    if (curr_x == px && curr_y == py)
    {
        // Immediately return true if the starting tile is the same as the target tile
        return true; // Reached target tile safely
    }

    for (uint8_t step = 0; step < 20; step++)
    {
        if (step > 0)
        {
            uint16_t tile_idx = (curr_y << shiftcount) + curr_x;
            if (!(map_collision_buf[tile_idx] & MAP_COLL_PASS_SIGHT))
            {
                return false; // Tile does not allow sight to pass
            }

            if (curr_x == px && curr_y == py)
            {
                return true; // Reached target tile safely
            }
        }

        int16_t next_x = curr_x;
        int16_t next_y = curr_y;

        int16_t e2 = (int16_t)((uint16_t)err << 1);
        if (e2 > -dy)
        {
            err -= dy;
            next_x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            next_y += sy;
        }

        // Super-cover corner check: if stepping diagonally, check orthogonal corner tiles to prevent corner-cutting
        if (next_x != curr_x && next_y != curr_y)
        {
            if (!Routines_Enemy_Ai_IsTileInBounds(next_x, curr_y) || !Routines_Enemy_Ai_IsTileInBounds(curr_x, next_y))
            {
                return false; // Out of bounds
            }

            uint16_t corner1_idx = (curr_y << shiftcount) + next_x;
            uint16_t corner2_idx = (next_y << shiftcount) + curr_x;

            if (!(map_collision_buf[corner1_idx] & MAP_COLL_PASS_SIGHT) || !(map_collision_buf[corner2_idx] & MAP_COLL_PASS_SIGHT))
            {
                return false; // Corner tile blocks diagonal sight line
            }
        }

        curr_x = next_x;
        curr_y = next_y;

        if (!Routines_Enemy_Ai_IsTileInBounds(curr_x, curr_y))
        {
            return false; // Out of bounds
        }
    }

    // Player is too far away (beyond 20 tiles) to be seen by enemy
    return false;
}

/*
    Line of sight check with vision hemisphere pre-check and metatile Bresenham raycast
*/
bool Routines_Enemy_Ai_CheckLineOfSight(struct game_object * o, int16_t x, int16_t y)
{
    struct game_data_npc * npc = &o->struct_data.npc_data;

    // x and y represent (enemy_x - player_x) and (enemy_y - player_y)
    uint8_t angle_to_player = Math_GetAtan2_u8(y, x);
    uint8_t angle_diff = (uint8_t)(angle_to_player - o->angle);

    // Unalerted enemies use a 180-degree forward vision hemisphere (+/- 90 degrees) for 2D top-down gameplay
    if ((npc->ai_flags & AI_FLAG_ALERTED) == 0)
    {
        if (angle_diff > 64 && angle_diff < 192)
        {
            return false; // Outside 180-degree forward vision hemisphere
        }
    }

    int16_t ex = (o->pos.x.lh.h + 8) >> 4;
    int16_t ey = (o->pos.y.lh.h + 8) >> 4;
    int16_t px = (o->pos.x.lh.h + 8 - x) >> 4;
    int16_t py = (o->pos.y.lh.h + 8 - y) >> 4;

    return Routines_Enemy_Ai_CheckRawTileLOS(ex, ey, px, py);
}

/*
    Propagate awareness to nearby unalerted enemies within radius_sq
*/
void Routines_Enemy_Ai_PropagateAwareness(struct game_object * source_o, uint32_t radius_sq)
{
    if (obj_active_count == 0)
    {
        return;
    }

    int16_t sx = source_o->pos.x.lh.h;
    int16_t sy = source_o->pos.y.lh.h;

    uint16_t processed = 0;
    struct game_object * other = &obj_general[0];
    
    for (uint16_t i = 0; i < OBJ_GENERAL_MAX_COUNT; i++)
    {
        uint16_t id = other->id;
        if (id != OBJID_NULL)
        {
            if (other != source_o && id >= OBJID_SLIME && id <= OBJID_LIZARDMAN_ARCHER)
            {
                if ((other->struct_data.npc_data.ai_flags & AI_FLAG_ALERTED) == 0)
                {
                    uint16_t state = other->state;
                    if (state != STATE_DIE && state != STATE_SPAWNING)
                    {
                        int16_t dx = sx - other->pos.x.lh.h;
                        int16_t dy = sy - other->pos.y.lh.h;

                        // Manhattan early-out: skip 32-bit math for objects clearly out of range
                        if (dx < 0) dx = -dx;
                        if (dy < 0) dy = -dy;
                        if (dx <= 256 && dy <= 256)
                        {
                            uint32_t dist_sq = Math_GetDistanceSquared(dx, dy);

                            if (dist_sq <= radius_sq)
                            {
                                other->struct_data.npc_data.ai_flags |= (AI_FLAG_ALERTED | AI_FLAG_LOS);
                            }
                        }
                    }
                }
            }
            processed++;
            if (processed >= obj_active_count)
            {
                break;
            }
        }
        other++;
    }
}

/*
    Gameplay AI for non-player entities with vision cone, obstacle detours, and archetypes
*/
bool Routines_Enemy_Ai_Process(struct game_object * o, uint32_t dist, int16_t x, int16_t y, uint32_t dist_min, bool allow_flipflop, uint16_t attack_delay, uint8_t archetype)
{
    struct game_data_npc * npc = &o->struct_data.npc_data;
    bool temp_invalidate_animation_frame = false;
    uint16_t temp_interrupted = 0;
    uint8_t ai_state = npc->ai_state;
    uint8_t prev_flags = npc->ai_flags;

    // Evaluate awareness & line-of-sight
    if (npc->invuln_time > 0 || npc->status == STATUS_BURNING)
    {
        // Invulnerable frame (damaged) or burning auto-alerts
        npc->ai_flags |= (AI_FLAG_ALERTED | AI_FLAG_LOS);
    }
    else if (dist <= 2304) // Peripheral / Hearing proximity (~48px / 3 tiles)
    {
        int16_t ex = (o->pos.x.lh.h + 8) >> 4;
        int16_t ey = (o->pos.y.lh.h + 8) >> 4;
        int16_t px = (o->pos.x.lh.h + 8 - x) >> 4;
        int16_t py = (o->pos.y.lh.h + 8 - y) >> 4;

        if (Routines_Enemy_Ai_CheckRawTileLOS(ex, ey, px, py))
        {
            npc->ai_flags |= (AI_FLAG_ALERTED | AI_FLAG_LOS);
        }
        else
        {
            npc->ai_flags &= ~AI_FLAG_LOS;
        }
    }
    else if ((((o->uid + (uint16_t)system_frames_elapsed) & 0x0f) == 0) || ai_state == AI_STATE_IDLE || ai_state == AI_STATE_WANDER)
    {
        if (Routines_Enemy_Ai_CheckLineOfSight(o, x, y))
        {
            npc->ai_flags |= (AI_FLAG_ALERTED | AI_FLAG_LOS);
        }
        else
        {
            npc->ai_flags &= ~AI_FLAG_LOS;
        }
    }

    // Group alert propagation: if enemy was just alerted, alert nearby pack members across full screen (256px radius = 65536 sq px)!
    if ((npc->ai_flags & AI_FLAG_ALERTED) && !(prev_flags & AI_FLAG_ALERTED))
    {
        Routines_Enemy_Ai_PropagateAwareness(o, 65536l);
    }

    // De-aggro mechanism: If alerted but LOS is lost while hitting an obstacle or timer expires, drop target!
    if ((npc->ai_flags & AI_FLAG_ALERTED) && !(npc->ai_flags & AI_FLAG_LOS))
    {
        if (npc->ai_stuck_count >= 2 || npc->ai_timer == 0)
        {
            // Clear alert state - give up on un-los target
            npc->ai_flags &= ~AI_FLAG_ALERTED;
            npc->ai_stuck_count = 0;
            npc->ai_state = AI_STATE_IDLE;
            o->delta.x.a = 0;
            o->delta.y.a = 0;
            o->state = STATE_IDLE;
            npc->ai_timer = (30 + ((uint8_t)Math_GetRandom_u16() & 0x1f)) / V_MUL;
            return true;
        }
    }

    // Handle wall collision detour steering when LOS is active
    if (npc->ai_stuck_count >= 3 && ai_state != AI_STATE_ATTACK)
    {
        if (npc->ai_tactic_dir == 0)
        {
            npc->ai_tactic_dir = ((uint8_t)Math_GetRandom_u16() & 0x01) ? 1 : 255;
        }
        uint8_t detour_angle = Math_GetAtan2_u8(y, x) + ((npc->ai_tactic_dir == 1) ? 64 : 192);
        o->angle = detour_angle;
        o->delta.x.a = Math_Cos(detour_angle) * V_MUL;
        o->delta.y.a = Math_Sin(detour_angle) * V_MUL;
        o->facing = Routines_Enemy_GetFacing(o);

        npc->ai_state = AI_STATE_DETOUR;
        npc->ai_timer = 20 / V_MUL;
        npc->ai_stuck_count = 0;
        return true;
    }

    // Interrupt check when aware
    if ((npc->ai_flags & AI_FLAG_ALERTED) && ai_state != AI_STATE_ATTACK)
    {
        if (((dist <= dist_min) && (ai_state == AI_STATE_MOVE_TOWARDS)) || (npc->status == STATUS_BURNING))
        {
            npc->ai_timer = 0;
            temp_interrupted = 1;
        }
        else if (!allow_flipflop)
        {
            if ((dist > dist_min) && (ai_state == AI_STATE_MOVE_AWAY))
            {
                npc->ai_timer = 0;
            }
        }
    }

    ai_state = npc->ai_state; // Refresh after possible mutations above

    if (npc->ai_timer == 0)
    {
        uint8_t temp_rand = (uint8_t)Math_GetRandom_u16();

        // Unalerted enemies: wander or stay idle (periodically turning around)
        if ((npc->ai_flags & AI_FLAG_ALERTED) == 0)
        {
            if (temp_rand < 192)
            {
                uint8_t wander_angle = temp_rand;
                o->angle = wander_angle;
                o->delta.x.a = Math_Cos(wander_angle);
                o->delta.y.a = Math_Sin(wander_angle);
                npc->ai_state = AI_STATE_WANDER;
                o->state = STATE_MOVE_WALK;
                o->facing = Routines_Enemy_GetFacing(o);
            }
            else
            {
                o->delta.x.a = 0;
                o->delta.y.a = 0;
                o->angle = temp_rand; // Turn to a new facing direction while idling
                npc->ai_state = AI_STATE_IDLE;
                o->state = STATE_IDLE;
                o->facing = Routines_Enemy_GetFacing(o);
            }

            npc->ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;
            temp_invalidate_animation_frame = true;
            return temp_invalidate_animation_frame;
        }

        // Aware enemies
        if (ai_state == AI_STATE_IDLE || ai_state == AI_STATE_WANDER || ai_state == AI_STATE_DETOUR || (temp_interrupted && (ai_state != AI_STATE_ATTACK)))
        {
            if ((dist > dist_min) && (npc->status != STATUS_BURNING))
            {
                uint8_t temp_angle = Math_GetAtan2_u8(y, x);
                if (archetype == AI_ARCHETYPE_CHASER)
                {
                    // Serpentine path + group fanning
                    temp_angle += ((temp_rand & 0x0f) - 8) + ((o->uid & 0x0f) - 8);
                }
                else
                {
                    temp_angle += (temp_rand & 0x0f) - 8;
                }

                o->angle = temp_angle;
                o->delta.x.a = Math_Cos(temp_angle) * V_MUL;
                o->delta.y.a = Math_Sin(temp_angle) * V_MUL;

                npc->ai_state = AI_STATE_MOVE_TOWARDS;
                o->state = (o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE || npc->status == STATUS_BURNING) ? STATE_HURT_BURN_MOVE : STATE_MOVE_WALK;
                o->facing = Routines_Enemy_GetFacing(o);
                temp_invalidate_animation_frame = true;
            }
            else if (ai_state != AI_STATE_MOVE_AWAY)
            {
                if (archetype == AI_ARCHETYPE_SKIRMISHER && (npc->ai_flags & AI_FLAG_LOS))
                {
                    // Strafe mode for ranged skirmishers
                    if (npc->ai_tactic_dir == 0)
                    {
                        npc->ai_tactic_dir = (temp_rand & 0x01) ? 1 : 255;
                    }
                    uint8_t strafe_angle = Math_GetAtan2_u8(y, x) + ((npc->ai_tactic_dir == 1) ? 64 : 192);
                    o->angle = strafe_angle;
                    o->delta.x.a = Math_Cos(strafe_angle) * V_MUL;
                    o->delta.y.a = Math_Sin(strafe_angle) * V_MUL;

                    npc->ai_state = AI_STATE_STRAFE;
                    o->state = (o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE || npc->status == STATUS_BURNING) ? STATE_HURT_BURN_MOVE : STATE_MOVE_WALK;
                    o->facing = Routines_Enemy_GetFacing(o);
                    temp_invalidate_animation_frame = true;
                }
                else
                {
                    // Move away
                    uint8_t temp_angle = Math_GetAtan2_u8(y, x) + (temp_rand & 0x0f) - 136;
                    o->angle = temp_angle;
                    o->delta.x.a = Math_Cos(temp_angle) * V_MUL;
                    o->delta.y.a = Math_Sin(temp_angle) * V_MUL;

                    npc->ai_state = AI_STATE_MOVE_AWAY;
                    o->state = (o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE || npc->status == STATUS_BURNING) ? STATE_HURT_BURN_MOVE : STATE_MOVE_WALK;
                    o->facing = Routines_Enemy_GetFacing(o);
                    temp_invalidate_animation_frame = true;
                }
            }

            npc->ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;
        }
        else if ((ai_state == AI_STATE_MOVE_TOWARDS) || (ai_state == AI_STATE_MOVE_AWAY) || (ai_state == AI_STATE_STRAFE))
        {
            o->delta.x.a = 0;
            o->delta.y.a = 0;

            if (ai_state == AI_STATE_MOVE_AWAY)
            {
                o->angle = o->angle + 128;
                o->facing = Routines_Enemy_GetFacing(o);
            }

            // Decide whether to attack: if LOS is blocked, suppress attack 75% of the time (don't bother)
            if (temp_rand >= 128 && ((npc->ai_flags & AI_FLAG_LOS) || (temp_rand >= 224)))
            {
                if (ai_state == AI_STATE_MOVE_TOWARDS)
                {
                    o->state = (dist <= dist_min) ? STATE_ATTACK_BASIC : STATE_ATTACK_SPECIAL;
                }
                else
                {
                    o->state = STATE_ATTACK_SPECIAL;
                }

                npc->ai_state = AI_STATE_ATTACK;
                npc->ai_timer = attack_delay;
                npc->ai_makeattack = 1;
            }
            else
            {
                npc->ai_state = AI_STATE_IDLE;
                o->state = (o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE || npc->status == STATUS_BURNING) ? STATE_HURT_BURN : STATE_IDLE;
                npc->ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;
            }

            temp_invalidate_animation_frame = true;
        }
        else if (ai_state == AI_STATE_ATTACK)
        {
            npc->ai_state = AI_STATE_IDLE;
            o->state = (o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE || npc->status == STATUS_BURNING) ? STATE_HURT_BURN : STATE_IDLE;
            npc->ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;
            temp_invalidate_animation_frame = true;
        }
    }

    if (npc->ai_timer > 0)
    {
        npc->ai_timer--;
    }

    return temp_invalidate_animation_frame;
}

/*
    Simple function that only reduces AI delay timer
*/
void Routines_Enemy_Ai_Idle(struct game_object * o)
{
    if (o->struct_data.npc_data.ai_timer > 0)
    {
        o->struct_data.npc_data.ai_timer--;
    }

    return;
}
