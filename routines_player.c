#include <stdlib.h>
#include <snes/console.h>

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

void routines_player(struct game_object * o)
{
    if (!system_game_paused)
    {
        if (snd_punch_timeout != 0)
        {
            snd_punch_timeout--;
        }

        if (snd_footstep_timeout != 0)
        {
            snd_footstep_timeout--;
        }

        uint16_t temp_nomove = 0;

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
                    temp_nomove = 1;
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

            if (system_check_for_key_hold(KEY_B))
            {
                if (!system_check_for_key_hold(KEY_Y))
                {
                    // Speed up the player if the player isn't attacking
                    o->state = STATE_MOVE_RUN;
                    o->delta.x.a *= 2;
                    o->delta.y.a *= 2;

                    temp_is_dashing = 1;
                }
            }

            // Test for what attack the player is trying 
            if (system_check_for_key(KEY_A))
            {
                // Punching
                // Update the animation state
                if (temp_nomove == 0)
                {
                    o->state = STATE_ATTACK_BASIC_MOVE;
                }
                else
                {
                    o->state = STATE_ATTACK_BASIC;
                }
                
                if (obj_player_attack_interval == 0)
                {
                    if (snd_punch_timeout == 0)
                    {
                        SoundInterface_PlaySfx(SFX_ATK_SWING, 0);

                        snd_punch_timeout = (8 / V_MUL);
                    }
                    
                    int16_t j = obj_instantiate_hitbox_player(OBJID_HITBOX_INVISIBLE, o->pos.x.lh.h, o->pos.y.lh.h);
                    if (j >= 0)
                    {
                        struct game_object * p = &obj_hitbox_player[j];
                        p->struct_data.npc_data.attack = PLAYER_ATTACK_VALUE * PLAYER_ATTACK_MULT_MELEE;

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

                        move_nocol_fast(p);

                        event_interaction_x = p->pos.x.lh.h;
                        event_interaction_y = p->pos.y.lh.h;
                    }
                }
            }
            else if (system_check_for_key_hold(KEY_Y))
            {
                // Fire breath
                // Update the animation state
                if (temp_nomove == 0)
                {
                    o->state = STATE_ATTACK_SPECIAL_MOVE;
                }
                else
                {
                    o->state = STATE_ATTACK_SPECIAL;
                }

                // Slow down the player
                o->delta.x.a /= 4;
                o->delta.y.a /= 4;

                temp_is_dashing = 0;

                if (obj_player_attack_interval == 0)
                {
                    int16_t j = obj_instantiate_hitbox_player(OBJID_FIREBALL, o->pos.x.lh.h, o->pos.y.lh.h);
                    if (j >= 0)
                    {
                        struct game_object * p = &obj_hitbox_player[j];
                        p->struct_data.npc_data.attack = PLAYER_ATTACK_VALUE * PLAYER_ATTACK_MULT_RANGED;

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

            if ((o->delta.x.a || o->delta.y.a) != 0)
            {
                move(o);

                if (snd_footstep_timeout == 0)
                {
                    SoundInterface_PlaySfx(SFX_MOV_FOOTSTEP,0);
                    //SoundInterface_PlaySfx_Ex(SFX_MOV_FOOTSTEP,127,127,255);
                    
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

            map_camera_adjust(0);

            uint16_t temp_invalidate_animation_frame = 0;

            if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
            {
                struct game_object * p = hit_test_player(o);
                if (p != NULL)
                {
                    SoundInterface_PlaySfx(SFX_ATK_SPLAT_HIT, 0);

                    // spawn an impact FX object
                    int16_t k = -1;
                    int16_t temp_x = o->pos.x.lh.h;
                    int16_t temp_y = o->pos.y.lh.h;

                    k = obj_instantiate(OBJID_SYS_IMPACT, temp_x, temp_y, 0);
                    
                    if (k >= 0)
                    {
                        struct game_object * q = &obj_general[k];

                        q->struct_data.npc_data.ttl = 2;
                    }

                    if (o->struct_data.npc_data.invuln_time == 0)
                    {
                        // If player is currently not invulnerable, trigger the damage
                        int32_t temp_dmg = (p->struct_data.npc_data.attack - o->struct_data.npc_data.defense);
                        
                        if (temp_dmg <= 0)
                        {
                            temp_dmg = 1;
                        }

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

                        // Also trigger the mosaic
                        gfx_mosaic_change = -1;
                        gfx_mosaic_intensity = 4;
                        gfx_mosaic_layers = 0x02;
                    }

                    o->struct_data.npc_data.ttl = 1; // despawn the object that triggered the hit

                    temp_invalidate_animation_frame = 1;
                }
            }

            if (o->struct_data.npc_data.invuln_time > 0)
            {
                o->struct_data.npc_data.invuln_time--;
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
            if (o->struct_data.npc_data.status_time == 0)
            {
                obj_destroy(o->array_index);

                // Initiate the soft reset handler
                // TODO: game over?

                system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_FADEOUT);
                system_target_routine = ROUTINE_RESET;
                shadow_inidisp = 0x0f;
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
                if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_4) == ANI_INTERVAL_4)
                {
                    o->struct_data.npc_data.ani.frame ^= 0x0001;
                }
                break;
            case STATE_DIE:
                if ((((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8) && (o->struct_data.npc_data.ani.frame < 7))
                {
                    o->struct_data.npc_data.ani.frame++;
                }
                break;
            default:
                // Update every 8 frames
                if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8)
                {
                    o->struct_data.npc_data.ani.frame ^= 0x0001;
                }
        }
    }

    uint8_t * temp_addr = ani_getframe_player(o);

    if ((temp_addr != o->struct_data.npc_data.ani.last_address))
    {
        // Save the requested frame into object's data
        // for comparison and in case it fails
        o->struct_data.npc_data.ani.last_address = temp_addr;

        if (dma_queue_add(temp_addr, 0x6000, 128, VRAM_INCHIGH, 1))
        {
            o->struct_data.npc_data.ani.last_dmafailed = 1;
        }
        else
        {
            o->struct_data.npc_data.ani.last_dmafailed = 0;
        }        
    }
    else if ((o->struct_data.npc_data.ani.last_dmafailed))
    {
        // The previous DMA failed. Attempt it again.
        if (dma_queue_add(o->struct_data.npc_data.ani.last_address, 0x6000, 128, VRAM_INCHIGH, 1) == 0)
        {
            o->struct_data.npc_data.ani.last_dmafailed = 0;
        }     
    }
    
    uint16_t temp_tileattrib = (o->struct_data.npc_data.ani.display | PAL_PLAYER << 9 | 2 << 12);

    if ((o->struct_data.npc_data.invuln_time != 0) && (((uint16_t)system_frames_elapsed & 0x01) == 0x01))
    {
        ;
    }
    else
    {
        SpriteEngine_AddToSortedLayer(o, temp_tileattrib);
    }

    return;
}

void routines_fireball(struct game_object * o)
{
    snd_flame_active = 1;

    if (!system_game_paused)
    {
        // The sound effect should only play when not paused
        if (snd_flame_playing == 0)
        {
            SoundInterface_PlaySfx(SFX_ATK_FIRE_BREATH, 0);

            snd_flame_playing = 1;
        }

        // Move the object based on the stored delta
        move_nocol_fast(o);

        // Update every 8 frames
        if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8)
        {
            o->struct_data.npc_data.ani.frame ^= 0x0001;

            o->struct_data.npc_data.ani.display = ani_getframe_fixed_fast(o);
        }

        // Decrement time to live
        o->struct_data.npc_data.ttl--;

        // Check if the object is to be destroyed
        if (o->struct_data.npc_data.ttl == 0)
        {
            obj_destroy_hitbox_player(o->array_index);
        }
    }

    // Only draw every other frame for both visibility and performance
    if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
    {
        SpriteEngine_AddToFrontLayer(o, (o->struct_data.npc_data.ani.display | PAL_FIREBALL << 9 | 3 << 12));
    }

    obj_player_active_fireballs++;

    return;
}


void routines_hitbox_invis(struct game_object * o)
{
    if (system_game_paused)
    {
        return;
    }

    // Decrement time to live
    o->struct_data.npc_data.ttl--;

    // Check if the object is to be destroyed
    if (o->struct_data.npc_data.ttl == 0)
    {
        obj_destroy_hitbox_player(o->array_index);
    }

    return;
}
