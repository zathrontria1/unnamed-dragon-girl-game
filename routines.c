#include <stdlib.h>
#include <snes/console.h>

#include "vars.h"

#include "spr.h"
#include "spr_metaspr.h"

#include "ani.h"

#include "obj.h"
#include "dma.h"
#include "routines.h"
#include "math_int.h"
#include "map.h"
#include "system.h"

#include "ui.h"

#include "data_strings.h"

#include "snd.h"
#include "consts_snd.h"

#include "hittest.h"

void routines_player(struct game_object * o)
{
    if (system_current_routine != ROUTINE_MSGBOX)
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
                        o->ani.frame = 0;
                    }
                    o->state = STATE_MOVE_WALK;
                    o->facing = FACING_LEFT;

                    temp_x--;

                    break;
                case KEY_RIGHT:
                    if (o->state == STATE_IDLE)
                    {
                        o->ani.frame = 0;
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
                        o->ani.frame = 0;
                    }
                    o->state = STATE_MOVE_WALK;
                    o->facing = FACING_UP;

                    temp_y--;

                    break;
                case KEY_DOWN:
                    if (o->state == STATE_IDLE)
                    {
                        o->ani.frame = 0;
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
                        snd_play_sfx(SFX_ATK_SWING, 0);

                        snd_punch_timeout = (8 / V_MUL);
                    }
                
                    
                    int16_t j = obj_instantiate(OBJID_HITBOX_INVISIBLE, o->pos.x.lh.h, o->pos.y.lh.h, 0);
                    if (j >= 0)
                    {
                        struct game_object * p = &objects[j];
                        p->attack = PLAYER_ATTACK_VALUE * PLAYER_ATTACK_MULT_MELEE;

                        p->ttl = 2; // Lasts exactly 2 frames

                        p->delta.x.a = 0;
                        p->delta.y.a = 0;

                        obj_player_attack_interval = PLAYER_ATTACK_INTERVAL_NORMAL;

                        switch (o->facing)
                        {
                            case FACING_UP:
                                p->pos.y.lh.h -= 16;
                                break;
                            case FACING_DOWN:
                                p->pos.y.lh.h += 16;
                                break;
                            case FACING_LEFT:
                                p->pos.x.lh.h -= 16;
                                break;
                            case FACING_RIGHT:
                                p->pos.x.lh.h += 16;
                                break;
                            case FACING_UPLEFT:
                                p->pos.x.lh.h -= 12;
                                p->pos.y.lh.h -= 12;
                                break;
                            case FACING_UPRIGHT:
                                p->pos.x.lh.h += 12;
                                p->pos.y.lh.h -= 12;
                                break;
                            case FACING_DOWNLEFT:
                                p->pos.x.lh.h -= 12;
                                p->pos.y.lh.h += 12;
                                break;
                            case FACING_DOWNRIGHT:
                                p->pos.x.lh.h += 12;
                                p->pos.y.lh.h += 12;
                                break;
                        }

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
                    int16_t j = obj_instantiate(OBJID_FIREBALL, o->pos.x.lh.h, o->pos.y.lh.h, 0);
                    if (j >= 0)
                    {
                        struct game_object * p = &objects[j];
                        p->attack = PLAYER_ATTACK_VALUE * PLAYER_ATTACK_MULT_RANGED;

                        int32_t temp_jitter = ((int32_t)((int16_t)rand_get16())) * (V_MUL * 2);

                        p->ttl = PLAYER_ATTACK_TTL;

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
                    o->ani.frame = 0;
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
                    snd_play_sfx(SFX_MOV_FOOTSTEP,0);
                    //snd_play_sfx_extend(SFX_MOV_FOOTSTEP,127,127,255);
                    
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
                    snd_play_sfx(SFX_ATK_SPLAT_HIT, 0);

                    // spawn an impact FX object
                    int16_t k = -1;
                    int16_t temp_x = o->pos.x.lh.h;
                    int16_t temp_y = o->pos.y.lh.h;
                    k = obj_instantiate(OBJID_SYS_IMPACT, temp_x, temp_y, 0);
                    
                    if (k >= 0)
                    {
                        struct game_object * q = &objects[k];

                        q->ttl = 2;
                    }

                    if (o->invuln_time == 0)
                    {
                        // If player is currently not invulnerable, trigger the damage
                        int32_t temp_dmg = (p->attack - o->defense);
                        if (temp_dmg <= 0)
                        {
                            temp_dmg = 1;
                        }
                        o->hp -= temp_dmg;

                        if (o->hp < 0)
                        {
                            o->hp = 0; 
                        }
                        else
                        {
                            // Give the player immunity
                            o->invuln_time = 60 / V_MUL;
                        }
                    }

                    p->ttl = 1; // despawn the object that triggered the hit

                    temp_invalidate_animation_frame = 1;
                }
            }

            if (o->invuln_time > 0)
            {
                o->invuln_time--;
            }

            // Check if the player is dead
            if ((o->hp <= 0) && (o->state != STATE_DIE))
            {
                o->state = STATE_DIE;
                o->ani.frame = 0;
                o->status_time = 64 / V_MUL;
                o->invuln_time = 0;
            }
        }
        else
        {
            o->status_time--;
            if (o->status_time == 0)
            {
                obj_destroy(o->array_index);

                // Initiate the soft reset handler
                // TODO: game over?
                system_current_routine = ROUTINE_FADEOUT;
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
                    o->ani.frame = 0;
                }
            }
        }

        switch (o->state)
        {
            case STATE_IDLE:
            case STATE_ATTACK_SPECIAL:
                o->ani.frame = 0;
                break;
            case STATE_MOVE_RUN:
                // Update every 4 frames
                if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_4) == ANI_INTERVAL_4)
                {
                    o->ani.frame ^= 0x0001;
                }
                break;
            case STATE_DIE:
                if ((((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8) && (o->ani.frame < 7))
                {
                    o->ani.frame++;
                }
                break;
            default:
                // Update every 8 frames
                if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8)
                {
                    o->ani.frame ^= 0x0001;
                }
        }
    }

    uint8_t * temp_addr = ani_getframe_player(o);

    if ((temp_addr != o->ani.last_address))
    {
        // Save the requested frame into object's data
        // for comparison and in case it fails
        o->ani.last_address = temp_addr;

        if (dma_queue_add(temp_addr, 0x6000, 128, VRAM_INCHIGH, 1))
        {
            o->ani.last_dmafailed = 1;
        }
        else
        {
            o->ani.last_dmafailed = 0;
        }        
    }
    else if ((o->ani.last_dmafailed))
    {
        // The previous DMA failed. Attempt it again.
        if (dma_queue_add(o->ani.last_address, 0x6000, 128, VRAM_INCHIGH, 1) == 0)
        {
            o->ani.last_dmafailed = 0;
        }     
    }
    
    uint16_t temp_tileattrib = (o->ani.display | PAL_PLAYER << 9 | 2 << 12);

    if ((o->invuln_time != 0) && (((uint16_t)system_frames_elapsed & 0x01) == 0x01))
    {
        ;
    }
    else
    {
        spr_queue_add_normal(o, temp_tileattrib);
    }

    return;
}

void routines_fireball(struct game_object * o)
{
    snd_flame_active = 1;

    if (system_current_routine != ROUTINE_MSGBOX)
    {
        // The sound effect should only play when not paused
        if (snd_flame_playing == 0)
        {
            snd_play_sfx(SFX_ATK_FIRE_BREATH, 0);

            snd_flame_playing = 1;
        }

        // Move the object based on the stored delta
        move_nocol_fast(o);

        // Update every 8 frames
        if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8)
        {
            o->ani.frame ^= 0x0001;

            o->ani.display = ani_getframe_fixed_fast(o);
        }

        // Decrement time to live
        o->ttl--;

        // Check if the object is to be destroyed
        if (o->ttl == 0)
        {
            obj_destroy(o->array_index);
        }
        else
        {
            hitbox_count_player++;
        }
    }

    // Only draw every other frame for both visibility and performance
    if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
    {
        spr_queue_add_front(o, (o->ani.display | PAL_FIREBALL << 9 | 3 << 12));
    }

    return;
}

void routines_fx_smoke(struct game_object * o)
{
    if (system_current_routine != ROUTINE_MSGBOX)
    {
        // Move the object based on the stored delta
        move_nocol_fast(o);

        // Update every 8 frames
        if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8)
        {
            o->ani.frame ^= 0x0001;

            o->ani.display = ani_getframe_fixed_fast(o);
        }

        // Decrement time to live
        o->ttl--;

        // Check if the object is to be destroyed
        if (o->ttl == 0)
        {
            obj_destroy(o->array_index);
        }
    }

    uint16_t temp_tileattrib = (o->ani.display | PAL_FX_SMOKE << 9 | 3 << 12);

    // Only draw every other frame for both visibility and performance
    if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
    {
        spr_queue_add_front(o, temp_tileattrib);
    }   

    return;
}

void routines_fx_impact(struct game_object * o)
{
    spr_queue_add_front(o, (ani_getframe_fixed_fast(o) | PAL_SYS_IMPACT << 9 | 3 << 12));

    if (system_current_routine == ROUTINE_MSGBOX)
    {
        return;
    }

    // Decrement time to live
    o->ttl--;

    // Check if the object is to be destroyed
    if (o->ttl == 0)
    {
        obj_destroy(o->array_index);
    }

    return;
}

void routines_hitbox_invis(struct game_object * o)
{
    if (system_current_routine == ROUTINE_MSGBOX)
    {
        return;
    }

    // Decrement time to live
    o->ttl--;

    // Check if the object is to be destroyed
    if (o->ttl == 0)
    {
        obj_destroy(o->array_index);
    }
    else
    {
        hitbox_count_player++;
    }

    return;
}

void routines_hitbox_invis_e(struct game_object * o)
{
    if (system_current_routine == ROUTINE_MSGBOX)
    {
        return;
    }

    // Decrement time to live
    o->ttl--;

    // Check if the object is to be destroyed
    if (o->ttl == 0)
    {
        obj_destroy(o->array_index);
    }
    else
    {
        hitbox_count_enemy++;
    }

    return;
}

void routines_interactable_switch(struct game_object * o)
{
    if (system_current_routine != ROUTINE_MSGBOX)
    {
        // Check if a player hit is on the switch

        // Only test when the switch can react (i.e. after timeout)
        // And while not in combat
        if (o->status_time == 0)
        {
            if (hit_test_interaction(o) != 0)
            {
                if (!event_in_combat_shadow)
                {
                    snd_play_sfx(SFX_INTERACT_SWITCH,0);
                    
                    o->state ^= STATE_SWITCH_ON;
                    event_flags_local[o->event_flag] = o->state;
                    o->status_time = 15 / V_MUL; // responsive enough for punch clicks

                    // spawn an impact FX object
                    int16_t k = -1;
                    int16_t temp_x = o->pos.x.lh.h;
                    int16_t temp_y = o->pos.y.lh.h;
                    k = obj_instantiate(OBJID_SYS_IMPACT, temp_x, temp_y, 0);
                    
                    if (k >= 0)
                    {
                        struct game_object * q = &objects[k];

                        q->ttl = 2;
                    }
                }
                else
                {
                    ui_print((uint8_t *)&STR_MSG_INCOMBAT, UI_MSGBOX_SL_START, UI_MARGIN_LEFT);
                }
            }
        }
        else
        {
            o->status_time--;
        }
    }

    spr_queue_add_back(o, (0x20 + (o->state << 1)) | PAL_INTERACTABLE_SWITCH_WALL << 9 | 2 << 12);

    return;
}

void routines_interactable_sign(struct game_object * o)
{
    if (system_current_routine != ROUTINE_MSGBOX)
    {
        // Check if a player hit is on the sign
        // And while not in combat
        if (o->status_time == 0)
        {
            if (hit_test_interaction(o) != 0)
            {
                if (!event_in_combat_shadow)
                {
                    snd_play_sfx(SFX_UI_CONFIRM, 0);

                    ui_print_ml(o->string_ptr, UI_MSGBOX_ML_START, UI_MARGIN_LEFT);

                    system_current_routine = ROUTINE_MSGBOX;
                    system_target_routine = ROUTINE_MSGBOX;

                    o->status_time = 15 / V_MUL; // responsive enough for punch clicks
                }
                else
                {
                    ui_print((uint8_t *)&STR_MSG_INCOMBAT, UI_MSGBOX_SL_START, UI_MARGIN_LEFT);
                }
            }
        }
        else
        {
            o->status_time--;
        }
    }

    spr_queue_add_back(o, 0x28 | PAL_INTERACTABLE_SIGN_WALL << 9 | 2 << 12);

    return;
}

void routines_interactable_blocker(struct game_object * o)
{
    o->state = event_flags_local[o->event_flag];

    if (o->state == STATE_SWITCH_OFF)
    {
        switch (o->id)
        {
            case OBJID_INTERACTABLE_BLOCKER_FLOOR:
                spr_queue_add_back(o, 0x0e | PAL_INTERACTABLE_BLOCKER_FLOOR << 9 | 2 << 12);
                break;
            case OBJID_INTERACTABLE_BLOCKER_DOOR_NS:
                spr_metaspr_draw(o, &data_metaspr_door_ns[0]);
                break;
            case OBJID_INTERACTABLE_BLOCKER_DOOR_EW:
                spr_metaspr_draw(o, &data_metaspr_door_ew[0]);
                break;
        }
        

        if (system_current_routine != ROUTINE_MSGBOX)
        {
            struct tile_xy temp;

            switch (o->id)
            {
                case OBJID_INTERACTABLE_BLOCKER_FLOOR:
                    blocker_list[blocker_build_count] = o->tile;
                    blocker_build_count++;
                    break;
                case OBJID_INTERACTABLE_BLOCKER_DOOR_NS:
                    blocker_list[blocker_build_count] = o->tile;
                    blocker_build_count++;
                    temp = o->tile;
                    temp.x = o->tile.x + 1;
                    blocker_list[blocker_build_count] = temp;
                    blocker_build_count++;
                    break;
                case OBJID_INTERACTABLE_BLOCKER_DOOR_EW:
                    blocker_list[blocker_build_count] = o->tile;
                    blocker_build_count++;
                    temp = o->tile;
                    temp.y = o->tile.y - 1;
                    blocker_list[blocker_build_count] = temp;
                    blocker_build_count++;
                    break;
            }
            
        }
    }

    return;
}

void routines_slime(struct game_object * o)
{
    if (system_current_routine != ROUTINE_MSGBOX)
    {
        uint16_t temp_invalidate_animation_frame = 0;

        // Note that all distance values obtained are squared for perf reasons (avoiding a square root)
        // This must be calculated regardless of AI state as the player uses this for
        // determining the closest target.
        struct game_object * p = &objects[obj_player_index];

        if ((o->state != STATE_DIE) && (o->state != STATE_SPAWNING))
        {
            if (o->hp_display_time > 0)
            {
                ui_show_enemy_health_bar(o);
            }

            int16_t temp_x = o->pos.x.lh.h - p->pos.x.lh.h;
            int16_t temp_y = o->pos.y.lh.h - p->pos.y.lh.h;

            uint32_t temp_dist = ai_distance_squared(temp_x, temp_y);

            // Range check - AI and sprite draws for enemies outside of visibility is ignored
            if (temp_dist < DIST_AI_MAX)
            {
                event_in_combat = 1;

                
                temp_invalidate_animation_frame = ai_run(o, temp_dist, temp_x, temp_y);

                // Move the object based on the stored delta
                if ((o->delta.x.a || o->delta.y.a) != 0)
                {
                    move(o);
                }

                // If the object is attacking:
                if (o->ai_makeattack)
                {
                    // Spawn a hit object
                    int16_t j = obj_instantiate(OBJID_BUBBLE_E, o->pos.x.lh.h, o->pos.y.lh.h, 0);

                    if (j >= 0)
                    {
                        snd_play_sfx(SFX_ATK_SPLASH,0);

                        struct game_object * p = &objects[j];
                        p->attack = ENEMY_ATTACK_VALUE * ENEMY_ATTACK_MULT_RANGED;
                        
                        p->delta.x.a = data_cosine_1[o->angle] * V_MUL;
                        p->delta.y.a = data_sine_1[o->angle] * V_MUL;

                        p->ttl = ENEMY_ATTACK_TTL;
                    }

                    o->ai_makeattack = 0;
                }

                // Stagger hit tests
                // Only non-invuln objects perform hit tests
                if (o->invuln_time == 0)
                {
                    if ((o->uid & 0x0001) == ((unsigned int)system_frames_elapsed & 0x0001))
                    {
                        struct game_object * p = hit_test_enemy(o);
                        if (p != NULL)
                        {
                            // spawn an impact FX object
                            int16_t k = -1;
                            int16_t temp_x = o->pos.x.lh.h;
                            int16_t temp_y = o->pos.y.lh.h;
                            k = obj_instantiate(OBJID_SYS_IMPACT, temp_x, temp_y, 0);
                            
                            if (k >= 0)
                            {
                                struct game_object * q = &objects[k];

                                q->ttl = 2;
                            }
                            
                            // A hit
                            // What triggered the hit?
                            if (p->id == OBJID_FIREBALL)
                            {
                                // DOT damage
                                o->status = STATUS_BURNING;
                                o->status_time = 60 / V_MUL;
                                o->invuln_time = 60 / V_MUL;

                                if (o->state == STATE_IDLE)
                                {
                                    o->state = STATE_HURT_BURN;
                                }
                                else
                                {
                                    o->state = STATE_HURT_BURN_MOVE;
                                }
                            }
                            else
                            {
                                // Single frame damage
                                o->invuln_time = 10 / V_MUL;

                                snd_play_sfx(SFX_ATK_PUNCH, 0);
                            }

                            long temp_dmg = (p->attack - o->defense);
                            if (temp_dmg <= 0)
                            {
                                temp_dmg = 1;
                            }
                            o->hp -= temp_dmg;

                            temp_invalidate_animation_frame = 1;

                            o->hp_display_time = 60 / V_MUL;
                        }
                    }
                }

                // burning objects produce vfx
                if (o->status == STATUS_BURNING)
                {
                    if (snd_firecrackle_timeout == 0)
                    {
                        snd_play_sfx(SFX_ATK_FIRE_CRACKLE, 0);

                        snd_firecrackle_timeout = (8 / V_MUL);
                    }

                    // Only spawn objects every 8 frames...
                    if (
                        (((uint16_t)system_frames_elapsed & FX_SMOKE_INTERVAL) == FX_SMOKE_INTERVAL)
                    )
                    {
                        int16_t k = -1;
                        int16_t temp_x = o->pos.x.lh.h;
                        int16_t temp_y = o->pos.y.lh.h;
                        k = obj_instantiate(OBJID_FX_SMOKE, temp_x, temp_y, 0);
                        
                        if (k >= 0)
                        {
                            struct game_object * q = &objects[k];

                            int32_t temp = ((int32_t)rand_get16() - 16384);

                            q->delta.x.a = temp;
                            q->delta.y.a = -(V_S_ONE / 2);

                            q->ttl = FX_SMOKE_TTL;
                        }
                    }
                }

                if (o->status_time > 0)
                {
                    o->status_time--;

                    if (o->status_time == 0)
                    {
                        o->status = STATUS_NORMAL;

                        if (o->state == STATE_HURT_BURN_MOVE)
                        {
                            o->state = STATE_MOVE_WALK;
                        }
                        else
                        {
                            o->state = STATE_IDLE;
                        }

                        temp_invalidate_animation_frame = 1;
                    }
                }

                if (o->invuln_time > 0)
                {
                    o->invuln_time--;
                }

                // Check if the slime is dead
                if ((o->hp <= 0) && (o->state != STATE_DIE))
                {
                    o->state = STATE_DIE;
                    o->ani.frame = 0;
                    o->status_time = 64 / V_MUL;
                }
            }
        }
        else if (o->state == STATE_DIE)
        {
            o->status_time--;
            if (o->status_time == 0)
            {
                int16_t k = -1;
                int16_t temp_x = o->pos.x.lh.h;
                int16_t temp_y = o->pos.y.lh.h;

                if ((int16_t)(rand_get16()) > (-24576))
                {
                    k = obj_instantiate(OBJID_DROP_MONEY, temp_x, temp_y, 0);

                    if (k >= 0)
                    {
                        struct game_object * q = &objects[k];

                        q->money = o->money;

                        q->pos.z.a = 0;
                        q->delta.z.a = (4 * V_S_ONE);
                    }
                }
                else
                {
                    k = obj_instantiate(OBJID_DROP_REC_MEAT, temp_x, temp_y, 0);

                    if (k >= 0)
                    {
                        struct game_object * q = &objects[k];

                        q->hp = ENEMY_DROP_REC_AMOUNT;

                        q->pos.z.a = 0;
                        q->delta.z.a = (4 * V_S_ONE);
                    }
                }

                obj_enemies_defeated++;
                obj_destroy(o->array_index);
                return;
            }
        }
        else // spawning
        {
            event_in_combat = 1;
            
            o->status_time--;
            if (o->status_time == 0)
            {
                o->state = STATE_IDLE;
            }
        }

        // Update every 8 frames
        switch (o->state)
        {
            case STATE_IDLE:
            case STATE_HURT_BURN:
                o->ani.frame = 0;
                break;
            case STATE_DIE:
            case STATE_SPAWNING:
                if ((((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8) && (o->ani.frame < 7))
                {
                    o->ani.frame++;
                }
                break;
            default:
                // Update every 8 frames
                if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8)
                {
                    o->ani.frame ^= 0x0001;
                }
        }

    }
    else
    {
        // Just draw the health bar if needed
        if ((o->state != STATE_DIE) && (o->state != STATE_SPAWNING))
        {
            if (o->hp_display_time > 0)
            {
                ui_show_enemy_health_bar(o);
            }
        }
    }

    // Testing DMA on demand
    uint8_t * temp_addr = ani_getframe_dynamic(o);

    if ((temp_addr != o->ani.last_address))
    {
        // Save the requested frame into object's data
        // for comparison and in case it fails
        o->ani.last_address = temp_addr;

        if (dma_queue_add(temp_addr, 0x6000+(o->vram_addr), 128, VRAM_INCHIGH, 1))
        {
            o->ani.last_dmafailed = 1;
        }
        else
        {
            o->ani.last_dmafailed = 0;
        }        
    }
    else if ((o->ani.last_dmafailed))
    {
        // The previous DMA failed. Attempt it again.
        if (!dma_queue_add(o->ani.last_address, 0x6000+(o->vram_addr), 128, VRAM_INCHIGH, 1))
        {
            o->ani.last_dmafailed = 0;
        }     
    }

    // DMA variant
    uint16_t temp_tileattrib;

    if (((uint32_t)temp_addr & 0x80000000) == 0x80000000) // sign bit is used for flip
    {
        temp_tileattrib = (o->tilenum | PAL_SLIME << 9 | 2 << 12 | 0x4000);
    }
    else
    {
        temp_tileattrib = (o->tilenum | PAL_SLIME << 9 | 2 << 12);
    }

    // Check if STAT77 is overflow
    if ((shadow_stat77 & 0x80) == 0x80)
    {
        // Do not perform a draw every other frame
        if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
        {
            spr_queue_add_normal(o, temp_tileattrib);
        }
    }
    else
    {
        spr_queue_add_normal(o, temp_tileattrib);
    }

    return;
}


void routines_bubble_e(struct game_object * o)
{
    if (system_current_routine != ROUTINE_MSGBOX)
    {
        // Move the object based on the stored delta
        move_nocol_fast(o);

        // Update every 8 frames
        if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8)
        {
            o->ani.frame ^= 0x0001;
        }

        // Decrement time to live
        o->ttl--;

        // Check if the object is to be destroyed
        if (o->ttl == 0)
        {
            obj_destroy(o->array_index);
        }
         else
        {
            hitbox_count_enemy++;
        }
    }

    uint8_t * temp_addr = ani_getframe_dynamic_stateless(o);

    if ((temp_addr != o->ani.last_address))
    {
        // Save the requested frame into object's data
        // for comparison and in case it fails
        o->ani.last_address = temp_addr;

        if (dma_queue_add(temp_addr, 0x6000+(o->vram_addr), 128, VRAM_INCHIGH, 1))
        {
            o->ani.last_dmafailed = 1;
        }
        else
        {
            o->ani.last_dmafailed = 0;
        }        
    }
    else if ((o->ani.last_dmafailed))
    {
        // The previous DMA failed. Attempt it again.
        if (!dma_queue_add(o->ani.last_address, 0x6000+(o->vram_addr), 128, VRAM_INCHIGH, 1))
        {
            o->ani.last_dmafailed = 0;
        }     
    }

    // Only draw every other frame for both visibility and performance
    if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
    {
        uint16_t temp_tileattrib;
        temp_tileattrib = (o->tilenum | PAL_BUBBLE_E << 9 | 3 << 12);

        spr_queue_add_front(o, temp_tileattrib);
    }

    return;
}

void routines_spawner(struct game_object * o)
{
    if (system_current_routine != ROUTINE_MSGBOX)
    {
        int16_t x1 = objects[obj_player_index].pos.x.lh.h;
        int16_t y1 = objects[obj_player_index].pos.y.lh.h;

        // Check if the player is within the designated box
        if (hit_test_extended(x1, o->spawn_area_x, y1, o->spawn_area_y, 16, o->spawn_area_w, 16, o->spawn_area_h) == 0)
        {
            obj_instantiate_npcs((struct obj_list_entry_spawns *)o->string_ptr, o->pos.x.lh.h, o->pos.y.lh.h);

            // Set the camera bounds
            bg_scroll_x_bounds_min.full.high.a = o->screen_x;
            bg_scroll_y_bounds_min.full.high.a = o->screen_y;

            bg_scroll_x_bounds_max.full.high.a = (o->screen_x + o->screen_w) - 256;
            bg_scroll_y_bounds_max.full.high.a = (o->screen_y + o->screen_h) - 224;

            if (bg_scroll_x_bounds_max.full.high.a < bg_scroll_x_bounds_min.full.high.a)
            {
                bg_scroll_x_bounds_max.full.high.a = bg_scroll_x_bounds_min.full.high.a;
            }
            if (bg_scroll_y_bounds_max.full.high.a < bg_scroll_y_bounds_min.full.high.a)
            {
                bg_scroll_y_bounds_max.full.high.a = bg_scroll_y_bounds_min.full.high.a;
            }

            bg_scroll_use_interpolation = 1;
            bg_scroll_suppress_interpolation_state_change = 2;

            obj_destroy(o->array_index);
        }
    }

    return;
}

void routines_drop_money(struct game_object * o)
{
    if (system_current_routine != ROUTINE_MSGBOX)
    {
        if (ani_animate_drop_gravity(o))
        {
            snd_play_sfx(SFX_DROP_COIN,0);
        }

        if (o->pos.z.a == 0) // If item is on floor
        {
            struct game_object * p = (struct game_object *)&objects[obj_player_index];

            // Check if the player is within the designated box
            if (hit_test(p, o) == 0)
            {
                snd_play_sfx(SFX_DROP_COIN,0);

                p->money += o->money;
                obj_destroy(o->array_index);
            }
        }
    }

    uint16_t temp_tileattrib;
    temp_tileattrib = (0x2a | PAL_DROP_MONEY << 9 | 2 << 12);

    spr_queue_add_normal(o, temp_tileattrib);

    return;
}

void routines_drop_rec_meat(struct game_object * o)
{
    if (system_current_routine != ROUTINE_MSGBOX)
    {
        if (ani_animate_drop_gravity(o))
        {
            snd_play_sfx(SFX_DROP_BOUNCE,0);
        }

        if (o->pos.z.a == 0) // If item is on floor
        {
            struct game_object * p = (struct game_object *)&objects[obj_player_index];

            // Check if the player is within the designated box
            if (hit_test(p, o) == 0)
            {
                snd_play_sfx(SFX_DROP_BOUNCE,0);
                
                if (p->hp + o->hp >= p->hp_max)
                {
                    p->hp = p->hp_max;
                }
                else
                {
                    p->hp += o->hp;
                }
            
                obj_destroy(o->array_index);
            }
        }
    }

    uint16_t temp_tileattrib;
    temp_tileattrib = (0x0c | PAL_DROP_REC_MEAT << 9 | 2 << 12);

    spr_queue_add_normal(o, temp_tileattrib);

    return;
}