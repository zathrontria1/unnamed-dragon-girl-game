#include <stdlib.h>
#include "snes/console.h"

#include "vars.h"

#include "spr.h"
#include "spr_metaspr.h"

#include "ani.h"

#include "obj.h"
#include "dma.h"
#include "routines.h"
#include "routines_player.h"
#include "math_int.h"
#include "map.h"
#include "system.h"

#include "ui.h"

#include "data_strings.h"

#include "snd.h"
#include "consts_snd.h"

#include "hittest.h"

#include "gfx.h"

#include "movement.h"

#include "main.h"

void Routines_Player(struct game_object * o)
{
    if (!system_game_paused)
    {
        if (snd_punch_timeout)
        {
            snd_punch_timeout--;
        }

        if (snd_footstep_timeout)
        {
            snd_footstep_timeout--;
        }

        bool temp_nomove = false;

        // Set event interaction to the most negative value
        event_interaction_x = -32728;
        event_interaction_y = -32728;

        if (o->state != STATE_DIE)
        {
            // Process player input
            uint16_t temp_x = 1;
            uint16_t temp_y = 1;

            switch (input_pad0 & (KEY_LEFT|KEY_RIGHT))
            {
                case KEY_LEFT:
                    if (o->state == STATE_IDLE)
                    {
                        o->struct_data.npc_data.ani.frame = 0;
                    }
                    o->state = STATE_MOVE_WALK;
                    o->facing = FACING_LEFT;

                    temp_x--;

                    break;
                case KEY_RIGHT:
                    if (o->state == STATE_IDLE)
                    {
                        o->struct_data.npc_data.ani.frame = 0;
                    }
                    o->state = STATE_MOVE_WALK;
                    o->facing = FACING_RIGHT;

                    temp_x++;

                    break;
            }

            switch (input_pad0 & (KEY_UP|KEY_DOWN))
            {
                case KEY_UP:
                    if (o->state == STATE_IDLE)
                    {
                        o->struct_data.npc_data.ani.frame = 0;
                    }
                    o->state = STATE_MOVE_WALK;
                    o->facing = FACING_UP;

                    temp_y--;

                    break;
                case KEY_DOWN:
                    if (o->state == STATE_IDLE)
                    {
                        o->struct_data.npc_data.ani.frame = 0;
                    }
                    o->state = STATE_MOVE_WALK;
                    o->facing = FACING_DOWN;

                    temp_y++;

                    break;
            }

            uint16_t temp_facing = FACING_KEEPSAME;
            uint16_t temp_deltacheck = temp_x | (temp_y << 2);

            switch (temp_deltacheck)
            {
                case 0:
                    o->delta.x.a = -V_S_DIAGONAL;
                    o->delta.y.a = -V_S_DIAGONAL;
                    temp_facing = FACING_UPLEFT;
                    break;
                case 1:
                    o->delta.x.a = 0;
                    o->delta.y.a = -V_S_ONE;
                    temp_facing = FACING_UP;
                    break;
                case 2:
                    o->delta.x.a = V_S_DIAGONAL;
                    o->delta.y.a = -V_S_DIAGONAL;
                    temp_facing = FACING_UPRIGHT;
                    break;
                case 4:
                    o->delta.x.a = -V_S_ONE;
                    o->delta.y.a = 0;
                    temp_facing = FACING_LEFT;
                    break;
                case 5:
                    o->delta.x.a = 0;
                    o->delta.y.a = 0;
                    temp_nomove = true;
                    break;
                case 6:
                    o->delta.x.a = V_S_ONE;
                    o->delta.y.a = 0;
                    temp_facing = FACING_RIGHT;
                    break;
                case 8:
                    o->delta.x.a = -V_S_DIAGONAL;
                    o->delta.y.a = V_S_DIAGONAL;
                    temp_facing = FACING_DOWNLEFT;
                    break;
                case 9:
                    o->delta.x.a = 0;
                    o->delta.y.a = V_S_ONE;
                    temp_facing = FACING_DOWN;
                    break;
                case 10:
                    o->delta.x.a = V_S_DIAGONAL;
                    o->delta.y.a = V_S_DIAGONAL;
                    temp_facing = FACING_DOWNRIGHT;
                    break;
            }

            if (temp_facing != FACING_KEEPSAME)
            {
                obj_player_prev_facing = temp_facing;
            }
            else
            {
                temp_facing = obj_player_prev_facing;
            }

            uint16_t temp_is_dashing = 0;

            if (System_CheckKeyHeld(KEY_B))
            {
                if (!System_CheckKeyHeld(KEY_Y))
                {
                    // Speed up the player if the player isn't attacking
                    o->state = STATE_MOVE_RUN;
                    o->delta.x.a *= 2;
                    o->delta.y.a *= 2;

                    temp_is_dashing = 1;
                }
            }

            // Test for what attack the player is trying 
            if (System_CheckKey(KEY_A))
            {
                // Punching
                // Update the animation state
                if (!temp_nomove)
                {
                    o->state = STATE_ATTACK_BASIC_MOVE;
                }
                else
                {
                    o->state = STATE_ATTACK_BASIC;
                }
                
                if (!obj_player_attack_interval)
                {
                    if (!snd_punch_timeout)
                    {
                        SoundInterface_PlaySfx_Pre(o, SFX_ATK_SWING);

                        snd_punch_timeout = (8 / V_MUL);
                    }
                    
                    int16_t j = ObjectSystem_InstantiatePlayerHitbox(OBJID_HITBOX_INVISIBLE, o->pos.x.lh.h, o->pos.y.lh.h);
                    if (j >= 0)
                    {
                        struct game_object * p = &obj_hitbox_player[j];
                        p->struct_data.npc_data.attack = o->struct_data.npc_data.attack * PLAYER_ATTACK_MULT_MELEE;

                        p->struct_data.npc_data.ttl = 2; // Lasts exactly 2 frames

                        p->delta.x.a = 0;
                        p->delta.y.a = 0;

                        obj_player_attack_interval = PLAYER_ATTACK_INTERVAL_NORMAL;

                        switch (o->facing)
                        {
                            case FACING_UP:
                                p->pos.y.lh.h -= 8;
                                p->w = 32;
                                break;
                            case FACING_DOWN:
                                p->pos.y.lh.h += 8;
                                p->w = 32;
                                break;
                            case FACING_LEFT:
                                p->pos.x.lh.h -= 8;
                                p->h = 32;
                                break;
                            case FACING_RIGHT:
                                p->pos.x.lh.h += 8;
                                p->h = 32;
                                break;
                            case FACING_UPLEFT:
                                p->pos.x.lh.h -= 6;
                                p->pos.y.lh.h -= 6;
                                p->w = 32;
                                p->h = 32;
                                break;
                            case FACING_UPRIGHT:
                                p->pos.x.lh.h += 6;
                                p->pos.y.lh.h -= 6;
                                p->w = 32;
                                p->h = 32;
                                break;
                            case FACING_DOWNLEFT:
                                p->pos.x.lh.h -= 6;
                                p->pos.y.lh.h += 6;
                                p->w = 32;
                                p->h = 32;
                                break;
                            case FACING_DOWNRIGHT:
                                p->pos.x.lh.h += 6;
                                p->pos.y.lh.h += 6;
                                p->w = 32;
                                p->h = 32;
                                break;
                        }

                        ObjectSystem_MoveWithoutCollision(p);

                        event_interaction_x = p->pos.x.lh.h;
                        event_interaction_y = p->pos.y.lh.h;

                        // Play a random voice here
                        SoundInterface_PlayClip(STREAM_VOICE_ATTACK_1 + (Math_GetRandom_u16() & 0x01));
                    }
                }
            }
            else if (System_CheckKeyHeld(KEY_Y))
            {
                // Fire breath
                // Update the animation state
                if (!temp_nomove)
                {
                    o->state = STATE_ATTACK_SPECIAL_MOVE;
                }
                else
                {
                    o->state = STATE_ATTACK_SPECIAL;
                }

                // Slow down the player
                o->delta.x.a >>= 2;
                o->delta.y.a >>= 2;

                temp_is_dashing = 0;

                if (!obj_player_attack_interval)
                {
                    int16_t j = ObjectSystem_InstantiatePlayerHitbox(OBJID_FIREBALL, o->pos.x.lh.h, o->pos.y.lh.h);
                    if (j >= 0)
                    {
                        struct game_object * p = &obj_hitbox_player[j];
                        p->struct_data.npc_data.attack = o->struct_data.npc_data.attack * PLAYER_ATTACK_MULT_RANGED;

                        int32_t temp_jitter = ((int32_t)((int16_t)Math_GetRandom_u16())) * (V_MUL * 2);

                        p->struct_data.npc_data.ttl = PLAYER_ATTACK_TTL;

                        if (temp_facing == FACING_KEEPSAME)
                        {
                            p->facing = o->facing;
                        }
                        else
                        {
                            p->facing = temp_facing;
                        }

                        switch (p->facing)
                        {
                            case FACING_UP:
                                p->pos.y.lh.h -= 8;
                                p->delta.x.a = temp_jitter;
                                p->delta.y.a = -V_S_ONE * 2;
                                break;
                            case FACING_DOWN:
                                p->pos.y.lh.h += 8;
                                p->delta.x.a = temp_jitter;
                                p->delta.y.a = V_S_ONE * 2;
                                break;
                            case FACING_LEFT:
                                p->pos.x.lh.h -= 8;
                                p->delta.x.a = -V_S_ONE * 2;
                                p->delta.y.a = temp_jitter;
                                break;
                            case FACING_RIGHT:
                                p->pos.x.lh.h += 8;
                                p->delta.x.a = V_S_ONE * 2;
                                p->delta.y.a = temp_jitter;
                                break;
                            case FACING_UPLEFT:
                                p->pos.x.lh.h -= 6;
                                p->pos.y.lh.h -= 6;
                                p->delta.x.a = -V_S_DIAGONAL * 2;
                                p->delta.y.a = -V_S_DIAGONAL * 2;
                                p->delta.y.a += temp_jitter;
                                break;
                            case FACING_UPRIGHT:
                                p->pos.x.lh.h += 6;
                                p->pos.y.lh.h -= 6;
                                p->delta.x.a = V_S_DIAGONAL * 2;
                                p->delta.y.a = -V_S_DIAGONAL * 2;
                                p->delta.y.a += temp_jitter;
                                break;
                            case FACING_DOWNLEFT:
                                p->pos.x.lh.h -= 6;
                                p->pos.y.lh.h += 6;
                                p->delta.x.a = -V_S_DIAGONAL * 2;
                                p->delta.y.a = V_S_DIAGONAL * 2;
                                p->delta.y.a += temp_jitter;
                                break;
                            case FACING_DOWNRIGHT:
                                p->pos.x.lh.h += 6;
                                p->pos.y.lh.h += 6;
                                p->delta.x.a = V_S_DIAGONAL * 2;
                                p->delta.y.a = V_S_DIAGONAL * 2;
                                p->delta.y.a += temp_jitter;
                                break;
                        }
                        
                    }

                    obj_player_attack_interval = PLAYER_ATTACK_INTERVAL_SPECIAL;
                }
            }
            else
            {
                if (
                    (o->state == STATE_ATTACK_SPECIAL) || 
                    (o->state == STATE_ATTACK_SPECIAL_MOVE) ||
                    (o->state == STATE_ATTACK_BASIC) || 
                    (o->state == STATE_ATTACK_BASIC_MOVE)) 
                {
                    o->struct_data.npc_data.ani.frame = 0;
                    o->state = STATE_IDLE;
                }
            }

            if (obj_player_attack_interval > 0)
            {
                obj_player_attack_interval--;
            }

            if (o->delta.x.a != 0 || o->delta.y.a != 0)
            {
                ObjectSystem_Move(o);

                if (snd_footstep_timeout == 0)
                {
                    SoundInterface_PlaySfx_Pre(o, SFX_MOV_FOOTSTEP);
                    
                    if (temp_is_dashing == 1)
                    {
                        snd_footstep_timeout = (8 / V_MUL); // Must follow tick size of this sound
                    }
                    else
                    {
                        snd_footstep_timeout = (16 / V_MUL); // Must follow tick size of this sound
                    }   
                }
            }

            MapSystem_UpdateCameraPosition(0);

            if (map_tilemap_recovery_pending)
            {
                return;
            }

            uint16_t temp_invalidate_animation_frame = 0;

            if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
            {
                struct game_object * p = CollisionCheck_PlayerTestEnemy(o);
                if (p != NULL)
                {
                    SoundInterface_PlaySfx_Pre(o, SFX_ATK_SPLAT_HIT);

                    // spawn an impact FX object
                    int16_t k = -1;
                    int16_t temp_x = o->pos.x.lh.h;
                    int16_t temp_y = o->pos.y.lh.h;

                    k = ObjectSystem_InstantiateObject(OBJID_SYS_IMPACT, temp_x, temp_y, 0);
                    
                    if (k >= 0)
                    {
                        struct game_object * q = &obj_general[k];

                        q->struct_data.npc_data.ttl = 2;
                    }

                    if (o->struct_data.npc_data.invuln_time == 0)
                    {
                        // If player is currently not invulnerable, trigger the damage
                        //int32_t temp_dmg = (p->struct_data.npc_data.attack - o->struct_data.npc_data.defense);
                        // debugging: make player invuln but still do the reads.
#if defined(DEBUG_PLAYER_IMMORTAL) || defined(DEBUG_ALL)
                        volatile int32_t temp_dmg = (p->struct_data.npc_data.attack - o->struct_data.npc_data.defense);
#else
                        int32_t temp_dmg = (p->struct_data.npc_data.attack - o->struct_data.npc_data.defense);
#endif
                        
                        if (temp_dmg <= 0)
                        {
                            temp_dmg = 1;
                        }

#if defined(DEBUG_PLAYER_IMMORTAL) || defined(DEBUG_ALL)
                        temp_dmg = 0; // debug
#endif

                        o->struct_data.npc_data.hp -= temp_dmg;

                        if (o->struct_data.npc_data.hp < 0)
                        {
                            o->struct_data.npc_data.hp = 0; 
                        }
                        else
                        {
                            // Give the player immunity
                            o->struct_data.npc_data.invuln_time = 60 / V_MUL;
                        }

                        obj_player_health_regen_delay = PLAYER_HEALTH_REGEN_DELAY;

                        // Also trigger the mosaic
                        gfx_mosaic_change = -1;
                        gfx_mosaic_intensity = 0x0400;
                        gfx_mosaic_layers = 0x02;
                    }

                    p->struct_data.npc_data.ttl = 1; // despawn the object that triggered the hit

                    temp_invalidate_animation_frame = 1;

                    SoundInterface_PlayClip(STREAM_VOICE_HURT_1 + (Math_GetRandom_u16() & 0x01));
                }
            }

            if (o->struct_data.npc_data.invuln_time > 0)
            {
                o->struct_data.npc_data.invuln_time--;
            }

            // Process health regen if needed

            if (!obj_player_health_regen_delay)
            {
                if (!obj_player_health_regen_interval)
                {
                    if (o->struct_data.npc_data.hp < obj_player_health_regen_limit)
                    {
                        obj_player_health_regen_interval = PLAYER_HEALTH_REGEN_INTERVAL;

                        o->struct_data.npc_data.hp += obj_player_health_regen_value;

                        if (o->struct_data.npc_data.hp > obj_player_health_regen_limit)
                        {
                            o->struct_data.npc_data.hp = obj_player_health_regen_limit;
                        }
                    }
                }
                else
                {
                    obj_player_health_regen_interval--;
                }
            }
            else
            {
                obj_player_health_regen_delay--;
            }

            // Check if the player is dead
            if ((o->struct_data.npc_data.hp <= 0) && (o->state != STATE_DIE))
            {
                o->state = STATE_DIE;
                o->struct_data.npc_data.ani.frame = 0;
                o->struct_data.npc_data.status_time = 64 / V_MUL;
                o->struct_data.npc_data.invuln_time = 0;
            }
        }
        else
        {
            o->struct_data.npc_data.status_time--;
            if (!o->struct_data.npc_data.status_time)
            {
                ObjectSystem_DestroyStandardObject(o->array_index);

                // Initiate the soft reset handler
                // TODO: game over?

                system_loop_func_ptr = Main_GetFunctionPointer(ROUTINE_FADEOUT);
                system_target_routine = ROUTINE_GAMEOVER;
                
                shadow_brightness = 15 << 8;
                return;
            }
        }
        
        if (o->state != STATE_DIE)
        {
            if ((temp_nomove) && !((o->state == STATE_ATTACK_SPECIAL) || o->state == STATE_ATTACK_BASIC))
            {
                if (o->state != STATE_IDLE)
                {
                    o->state = STATE_IDLE;
                    o->struct_data.npc_data.ani.frame = 0;
                }
            }
        }

        switch (o->state)
        {
            case STATE_IDLE:
            case STATE_ATTACK_SPECIAL:
                o->struct_data.npc_data.ani.frame = 0;
                break;
            case STATE_MOVE_RUN:
                // Update every 4 frames
                if (!((uint16_t)system_frames_elapsed & ANI_INTERVAL_4))
                {
                    o->struct_data.npc_data.ani.frame ^= 0x0001;
                }
                break;
            case STATE_DIE:
                if (!(((uint16_t)system_frames_elapsed & ANI_INTERVAL_8)) && (o->struct_data.npc_data.ani.frame < 7))
                {
                    o->struct_data.npc_data.ani.frame++;
                }
                break;
            default:
                // Update every 8 frames
                if (!((uint16_t)system_frames_elapsed & ANI_INTERVAL_8))
                {
                    o->struct_data.npc_data.ani.frame ^= 0x0001;
                }
        }
    }

    uint8_t * temp_addr = AniSystem_GetPlayerFrame(o); // This will cause a compressed frame fetch
    
    if ((o->struct_data.npc_data.invuln_time) && (((uint16_t)system_frames_elapsed & 0x01) == 0x01))
    {
        ;
    }
    else
    {
        Routines_Shared_Draw(o, temp_addr, PAL_PLAYER << 9, 1, false, true);
    }

    return;
}

void Routines_Player_Fireball(struct game_object * o)
{
    snd_flame_active = 1;

    if (!system_game_paused)
    {
        // The sound effect should only play when not paused
        if (!snd_flame_playing)
        {
            SoundInterface_PlaySfx(SFX_ATK_FIRE_BREATH, 0);

            snd_flame_playing = 1;
        }

        // Move the object based on the stored delta
        ObjectSystem_MoveWithoutCollision(o);

        // Update every 8 frames
        if (!((uint16_t)system_frames_elapsed & ANI_INTERVAL_8))
        {
            o->struct_data.npc_data.ani.frame ^= 0x0001;
        }

        // Decrement time to live
        o->struct_data.npc_data.ttl--;

        // Check if the object is to be destroyed
        if (!o->struct_data.npc_data.ttl)
        {
            ObjectSystem_DestroyPlayerHitbox(o->array_index);
        }
    }

    unsigned int offset = o->struct_data.npc_data.ani.frame << 1;

    Routines_Shared_DrawFixed(o, (0x0002 + offset) | PAL_FIREBALL << 9, 0, true); 

    obj_player_active_fireballs++;

    return;
}


void Routines_Player_InvisibleHit(struct game_object * o)
{
    if (system_game_paused)
    {
        return;
    }

    // Decrement time to live
    o->struct_data.npc_data.ttl--;

    // Check if the object is to be destroyed
    if (!o->struct_data.npc_data.ttl)
    {
        ObjectSystem_DestroyPlayerHitbox(o->array_index);
    }

    return;
}
