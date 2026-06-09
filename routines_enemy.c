#include <stdlib.h>
#include <snes/console.h>

#include "vars.h"

#include "spr.h"
#include "spr_metaspr.h"

#include "ani.h"

#include "obj.h"
#include "dma.h"
#include "routines.h"
#include "routines_enemy.h"
#include "routines_enemy_ai.h"
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

void routines_slime(struct game_object * o)
{
    if (!system_game_paused)
    {
        uint16_t temp_invalidate_animation_frame = 0;

        // Note that all distance values obtained are squared for perf reasons (avoiding a square root)
        // This must be calculated regardless of AI state as the player uses this for
        // determining the closest target.
        struct game_object * p = obj_player_pointer;

        if ((o->state != STATE_DIE) && (o->state != STATE_SPAWNING))
        {
            if (o->struct_data.npc_data.hp_display_time > 0)
            {
                UserInterface_DrawEnemyHealthBar(o);
            }

            int16_t temp_x = o->pos.x.lh.h - p->pos.x.lh.h;
            int16_t temp_y = o->pos.y.lh.h - p->pos.y.lh.h;

            uint32_t temp_dist = Math_GetDistanceSquared(temp_x, temp_y);

            // Range check - AI and sprite draws for enemies outside of visibility is ignored
            if (temp_dist < DIST_AI_MAX)
            {
                event_in_combat = 1;
                
                temp_invalidate_animation_frame = ai_run(o, temp_dist, temp_x, temp_y, DIST_MELEE, true, 15 / V_MUL);

                // Move the object based on the stored delta
                if ((o->delta.x.a || o->delta.y.a) != 0)
                {
                    move(o);
                }

                // If the object is attacking:
                if (o->struct_data.npc_data.ai_makeattack)
                {
                    // Spawn a hit object
                    int16_t j = obj_instantiate_hitbox_enemy(OBJID_BUBBLE_E, o->pos.x.lh.h, o->pos.y.lh.h);

                    if (j >= 0)
                    {
                        SoundInterface_PlaySfx(SFX_ATK_SPLASH,0);

                        struct game_object * p = &obj_hitbox_enemy[j];
                        p->struct_data.npc_data.attack = o->struct_data.npc_data.attack * ENEMY_ATTACK_MULT_RANGED;

                        // Angle needs to be recalculated
                        uint8_t temp_rand = (uint8_t)Math_GetRandom_u16();
                        uint8_t temp_angle = Math_GetAtan2_u8(temp_y, temp_x) + (temp_rand & 0x0f) - 8;

                        o->angle = temp_angle;
                        o->facing = ai_get_facing(o);
                        
                        p->delta.x.a = data_cosine_1[temp_angle] * V_MUL;
                        p->delta.y.a = data_sine_1[temp_angle] * V_MUL;

                        p->struct_data.npc_data.ttl = ENEMY_ATTACK_TTL;
                    }

                    o->struct_data.npc_data.ai_makeattack = 0;
                }

                // Stagger hit tests
                // Only non-invuln objects perform hit tests
                if (o->struct_data.npc_data.invuln_time == 0)
                {
                    if ((o->uid & 0x0001) == ((unsigned int)system_frames_elapsed & 0x0001))
                    {
                        struct game_object * p = CollisionCheck_EnemyTestPlayer(o);
                        if (p != NULL)
                        {
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
                            
                            // A hit
                            // What triggered the hit?
                            if (p->id == OBJID_FIREBALL)
                            {
                                // DOT damage
                                o->struct_data.npc_data.status = STATUS_BURNING;
                                o->struct_data.npc_data.status_time = 60 / V_MUL;
                                o->struct_data.npc_data.invuln_time = 60 / V_MUL;

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
                                o->struct_data.npc_data.invuln_time = 10 / V_MUL;

                                SoundInterface_PlaySfx(SFX_ATK_PUNCH, 0);
                            }

                            long temp_dmg = (p->struct_data.npc_data.attack - o->struct_data.npc_data.defense);
                            if (temp_dmg <= 0)
                            {
                                temp_dmg = 1;
                            }
                            o->struct_data.npc_data.hp -= temp_dmg;

                            temp_invalidate_animation_frame = 1;

                            o->struct_data.npc_data.hp_display_time = 60 / V_MUL;
                        }
                    }
                }

                // burning objects produce vfx
                if (o->struct_data.npc_data.status == STATUS_BURNING)
                {
                    if (snd_firecrackle_timeout == 0)
                    {
                        SoundInterface_PlaySfx(SFX_ATK_FIRE_CRACKLE, 0);

                        snd_firecrackle_timeout = (8 / V_MUL);
                    }

                    // Only spawn objects every 8 frames...
                    if (
                        (((uint16_t)(system_frames_elapsed) & FX_SMOKE_INTERVAL) == FX_SMOKE_INTERVAL)
                    )
                    {
                        int16_t temp_x = o->pos.x.lh.h;
                        int16_t temp_y = o->pos.y.lh.h;
                        int16_t k = obj_instantiate(OBJID_FX_SMOKE, temp_x, temp_y, 0);
                        
                        if (k >= 0)
                        {
                            struct game_object * q = &obj_general[k];

                            int32_t temp = ((int32_t)Math_GetRandom_u16() - 16384);

                            q->delta.x.a = temp;
                            q->delta.y.a = -(V_S_ONE / 2);

                            q->struct_data.npc_data.ttl = FX_SMOKE_TTL;
                        }
                    }
                }

                if (o->struct_data.npc_data.status_time > 0)
                {
                    o->struct_data.npc_data.status_time--;

                    if (o->struct_data.npc_data.status_time == 0)
                    {
                        o->struct_data.npc_data.status = STATUS_NORMAL;

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

                if (o->struct_data.npc_data.invuln_time > 0)
                {
                    o->struct_data.npc_data.invuln_time--;
                }

                // Check if the slime is dead
                if ((o->struct_data.npc_data.hp <= 0) && (o->state != STATE_DIE))
                {
                    o->state = STATE_DIE;
                    o->struct_data.npc_data.ani.frame = 0;
                    o->struct_data.npc_data.status_time = 64 / V_MUL;
                }
            }
        }
        else if (o->state == STATE_DIE)
        {
            o->struct_data.npc_data.status_time--;
            if (o->struct_data.npc_data.status_time == 0)
            {
                int16_t k = -1;
                int16_t temp_x = o->pos.x.lh.h;
                int16_t temp_y = o->pos.y.lh.h;

                if (((int16_t)(Math_GetRandom_u16()) > -24576) && (obj_player_recovery_drop_pity != 0))
                {
                    obj_player_recovery_drop_pity--;

                    k = obj_instantiate(OBJID_DROP_MONEY, temp_x, temp_y, 0);

                    if (k >= 0)
                    {
                        struct game_object * q = &obj_general[k];

                        q->struct_data.npc_data.money = o->struct_data.npc_data.money;

                        q->pos.z.a = 0;
                        q->delta.z.a = (4 * V_S_ONE);
                    }
                }
                else
                {   
                    obj_player_recovery_drop_pity = ENEMY_DROP_REC_PITY;
                    k = obj_instantiate(OBJID_DROP_REC_MEAT, temp_x, temp_y, 0);

                    if (k >= 0)
                    {
                        struct game_object * q = &obj_general[k];

                        q->struct_data.npc_data.hp = ENEMY_DROP_REC_AMOUNT;

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
            
            o->struct_data.npc_data.status_time--;
            if (o->struct_data.npc_data.status_time == 0)
            {
                o->state = STATE_IDLE;
            }
        }

        // Update every 8 frames
        switch (o->state)
        {
            case STATE_IDLE:
            case STATE_HURT_BURN:
                o->struct_data.npc_data.ani.frame = 0;
                break;
            case STATE_DIE:
            case STATE_SPAWNING:
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
    else
    {
        // Just draw the health bar if needed
        if ((o->state != STATE_DIE) && (o->state != STATE_SPAWNING))
        {
            if (o->struct_data.npc_data.hp_display_time > 0)
            {
                UserInterface_DrawEnemyHealthBar(o);
            }
        }
    }

    // Testing DMA on demand
    uint8_t * temp_addr = AniSystem_GetDynamicFrame_Slime(o);

    if ((temp_addr != o->struct_data.npc_data.ani.last_address))
    {
        // Save the requested frame into object's data
        // for comparison and in case it fails
        o->struct_data.npc_data.ani.last_address = temp_addr;

        if (DmaSystem_AddItemToQueue(temp_addr, 0x6000+(o->struct_data.npc_data.vram_addr), 128, VRAM_INCHIGH, 1))
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
        if (!DmaSystem_AddItemToQueue(o->struct_data.npc_data.ani.last_address, 0x6000+(o->struct_data.npc_data.vram_addr), 128, VRAM_INCHIGH, 1))
        {
            o->struct_data.npc_data.ani.last_dmafailed = 0;
        }     
    }

    // DMA variant
    uint16_t temp_tileattrib;

    if (((uint32_t)temp_addr & 0x80000000) == 0x80000000) // sign bit is used for flip
    {
        temp_tileattrib = (o->struct_data.npc_data.tilenum | PAL_SLIME << 9 | 2 << 12 | 0x4000);
    }
    else
    {
        temp_tileattrib = (o->struct_data.npc_data.tilenum | PAL_SLIME << 9 | 2 << 12);
    }

    // Check if STAT77 is overflow
    if ((shadow_stat77 & 0x80) == 0x80)
    {
        // Do not perform a draw every other frame
        if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
        {
            SpriteEngine_AddToSortedLayer(o, temp_tileattrib);
        }
    }
    else
    {
        SpriteEngine_AddToSortedLayer(o, temp_tileattrib);
    }

    return;
}

void routines_lizardman(struct game_object * o)
{
    if (!system_game_paused)
    {
        uint16_t temp_invalidate_animation_frame = 0;

        // Note that all distance values obtained are squared for perf reasons (avoiding a square root)
        // This must be calculated regardless of AI state as the player uses this for
        // determining the closest target.
        struct game_object * p = obj_player_pointer;

        if ((o->state != STATE_DIE) && (o->state != STATE_SPAWNING))
        {
            if (o->struct_data.npc_data.hp_display_time > 0)
            {
                UserInterface_DrawEnemyHealthBar(o);
            }

            int16_t temp_x = o->pos.x.lh.h - p->pos.x.lh.h;
            int16_t temp_y = o->pos.y.lh.h - p->pos.y.lh.h;

            uint32_t temp_dist = Math_GetDistanceSquared(temp_x, temp_y);

            // Range check - AI and sprite draws for enemies outside of visibility is ignored
            if (temp_dist < DIST_AI_MAX)
            {
                event_in_combat = 1;
                
                temp_invalidate_animation_frame = ai_run(o, temp_dist, temp_x, temp_y, DIST_TARGET_RANGE, false, 60 / V_MUL);

                // Move the object based on the stored delta
                if ((o->delta.x.a || o->delta.y.a) != 0)
                {
                    move(o);
                }

                // If the object is attacking:
                if (o->struct_data.npc_data.ai_makeattack)
                {
                    SoundInterface_PlayClip(STREAM_HISS);

                    // Spawn a hit object
                    int16_t j = obj_instantiate_hitbox_enemy(OBJID_ARROW_E, o->pos.x.lh.h, o->pos.y.lh.h);

                    if (j >= 0)
                    {
                        SoundInterface_PlaySfx(SFX_ATK_SWING,0);

                        struct game_object * p = &obj_hitbox_enemy[j];
                        p->struct_data.npc_data.attack = o->struct_data.npc_data.attack * ENEMY_ATTACK_MULT_RANGED;

                        // Angle needs to be recalculated
                        uint8_t temp_rand = (uint8_t)Math_GetRandom_u16();
                        uint8_t temp_angle = Math_GetAtan2_u8(temp_y, temp_x) + (temp_rand & 0x0f) - 8;

                        o->angle = temp_angle;
                        o->facing = ai_get_facing(o);

                        p->angle = temp_angle;
                        
                        p->delta.x.a = data_cosine_1[temp_angle] * V_MUL * 2;
                        p->delta.y.a = data_sine_1[temp_angle] * V_MUL * 2;

                        p->struct_data.npc_data.ttl = ENEMY_ATTACK_TTL;
                    }

                    o->struct_data.npc_data.ai_makeattack = 0;
                }

                // Stagger hit tests
                // Only non-invuln objects perform hit tests
                if (o->struct_data.npc_data.invuln_time == 0)
                {
                    if ((o->uid & 0x0001) == ((unsigned int)system_frames_elapsed & 0x0001))
                    {
                        struct game_object * p = CollisionCheck_EnemyTestPlayer(o);
                        if (p != NULL)
                        {
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
                            
                            // A hit
                            // What triggered the hit?
                            if (p->id == OBJID_FIREBALL)
                            {
                                // DOT damage
                                o->struct_data.npc_data.status = STATUS_BURNING;
                                o->struct_data.npc_data.status_time = 60 / V_MUL;
                                o->struct_data.npc_data.invuln_time = 60 / V_MUL;

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
                                o->struct_data.npc_data.invuln_time = 10 / V_MUL;

                                SoundInterface_PlaySfx(SFX_ATK_PUNCH, 0);
                            }

                            long temp_dmg = (p->struct_data.npc_data.attack - o->struct_data.npc_data.defense);
                            if (temp_dmg <= 0)
                            {
                                temp_dmg = 1;
                            }
                            o->struct_data.npc_data.hp -= temp_dmg;

                            temp_invalidate_animation_frame = 1;

                            o->struct_data.npc_data.hp_display_time = 60 / V_MUL;
                        }
                    }
                }

                // burning objects produce vfx
                if (o->struct_data.npc_data.status == STATUS_BURNING)
                {
                    if (snd_firecrackle_timeout == 0)
                    {
                        SoundInterface_PlaySfx(SFX_ATK_FIRE_CRACKLE, 0);

                        snd_firecrackle_timeout = (8 / V_MUL);
                    }

                    // Only spawn objects every 8 frames...
                    if (
                        (((uint16_t)(system_frames_elapsed) & FX_SMOKE_INTERVAL) == FX_SMOKE_INTERVAL)
                    )
                    {
                        int16_t temp_x = o->pos.x.lh.h;
                        int16_t temp_y = o->pos.y.lh.h;
                        int16_t k = obj_instantiate(OBJID_FX_SMOKE, temp_x, temp_y, 0);
                        
                        if (k >= 0)
                        {
                            struct game_object * q = &obj_general[k];

                            int32_t temp = ((int32_t)Math_GetRandom_u16() - 16384);

                            q->delta.x.a = temp;
                            q->delta.y.a = -(V_S_ONE / 2);

                            q->struct_data.npc_data.ttl = FX_SMOKE_TTL;
                        }
                    }
                }

                if (o->struct_data.npc_data.status_time > 0)
                {
                    o->struct_data.npc_data.status_time--;

                    if (o->struct_data.npc_data.status_time == 0)
                    {
                        o->struct_data.npc_data.status = STATUS_NORMAL;

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

                if (o->struct_data.npc_data.invuln_time > 0)
                {
                    o->struct_data.npc_data.invuln_time--;
                }

                // Check if the slime is dead
                if ((o->struct_data.npc_data.hp <= 0) && (o->state != STATE_DIE))
                {
                    o->state = STATE_DIE;
                    o->struct_data.npc_data.ani.frame = 0;
                    o->struct_data.npc_data.status_time = 64 / V_MUL;
                }
            }
        }
        else if (o->state == STATE_DIE)
        {
            o->struct_data.npc_data.status_time--;
            if (o->struct_data.npc_data.status_time == 0)
            {
                int16_t k = -1;
                int16_t temp_x = o->pos.x.lh.h;
                int16_t temp_y = o->pos.y.lh.h;

                if (((int16_t)(Math_GetRandom_u16()) > -24576) && (obj_player_recovery_drop_pity != 0))
                {
                    obj_player_recovery_drop_pity--;

                    k = obj_instantiate(OBJID_DROP_MONEY, temp_x, temp_y, 0);

                    if (k >= 0)
                    {
                        struct game_object * q = &obj_general[k];

                        q->struct_data.npc_data.money = o->struct_data.npc_data.money;

                        q->pos.z.a = 0;
                        q->delta.z.a = (4 * V_S_ONE);
                    }
                }
                else
                {   
                    obj_player_recovery_drop_pity = ENEMY_DROP_REC_PITY;
                    k = obj_instantiate(OBJID_DROP_REC_MEAT, temp_x, temp_y, 0);

                    if (k >= 0)
                    {
                        struct game_object * q = &obj_general[k];

                        q->struct_data.npc_data.hp = ENEMY_DROP_REC_AMOUNT;

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
            
            o->struct_data.npc_data.status_time--;
            if (o->struct_data.npc_data.status_time == 0)
            {
                o->state = STATE_IDLE;
            }
        }

        // Update every 8 frames
        switch (o->state)
        {
            case STATE_IDLE:
            case STATE_HURT_BURN:
            case STATE_ATTACK_SPECIAL:
            case STATE_ATTACK_SPECIAL_MOVE:
                o->struct_data.npc_data.ani.frame = 0;
                break;
            case STATE_DIE:
            case STATE_SPAWNING:
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
    else
    {
        // Just draw the health bar if needed
        if ((o->state != STATE_DIE) && (o->state != STATE_SPAWNING))
        {
            if (o->struct_data.npc_data.hp_display_time > 0)
            {
                UserInterface_DrawEnemyHealthBar(o);
            }
        }
    }

    // Testing DMA on demand
    uint8_t * temp_addr = AniSystem_GetDynamicFrame_Lizardman(o);

    if ((temp_addr != o->struct_data.npc_data.ani.last_address))
    {
        // Save the requested frame into object's data
        // for comparison and in case it fails
        o->struct_data.npc_data.ani.last_address = temp_addr;

        if (DmaSystem_AddItemToQueue(temp_addr, 0x6000+(o->struct_data.npc_data.vram_addr), 128, VRAM_INCHIGH, 1))
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
        if (!DmaSystem_AddItemToQueue(o->struct_data.npc_data.ani.last_address, 0x6000+(o->struct_data.npc_data.vram_addr), 128, VRAM_INCHIGH, 1))
        {
            o->struct_data.npc_data.ani.last_dmafailed = 0;
        }     
    }

    // DMA variant
    uint16_t temp_tileattrib;

    if (((uint32_t)temp_addr & 0x80000000) == 0x80000000) // sign bit is used for flip
    {
        temp_tileattrib = (o->struct_data.npc_data.tilenum | PAL_LIZARDMAN << 9 | 2 << 12 | 0x4000);
    }
    else
    {
        temp_tileattrib = (o->struct_data.npc_data.tilenum | PAL_LIZARDMAN << 9 | 2 << 12);
    }

    // Check if STAT77 is overflow
    if ((shadow_stat77 & 0x80) == 0x80)
    {
        // Do not perform a draw every other frame
        if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
        {
            SpriteEngine_AddToSortedLayer(o, temp_tileattrib);
        }
    }
    else
    {
        SpriteEngine_AddToSortedLayer(o, temp_tileattrib);
    }

    return;
}

void routines_bubble_e(struct game_object * o)
{
    if (!system_game_paused)
    {
        // Move the object based on the stored delta
        move_nocol_fast(o);

        // Update every 8 frames
        if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8)
        {
            o->struct_data.npc_data.ani.frame ^= 0x0001;
        }

        // Check if the object is to be destroyed
        if (o->struct_data.npc_data.ttl == 0)
        {
            obj_destroy_hitbox_enemy(o->array_index);
        }
        else
        {
            // Decrement time to live
            o->struct_data.npc_data.ttl--;
        }
    }

    uint8_t * temp_addr = AniSystem_GetDynamicFrame_Bubble(o);

    if ((temp_addr != o->struct_data.npc_data.ani.last_address))
    {
        // Save the requested frame into object's data
        // for comparison and in case it fails
        o->struct_data.npc_data.ani.last_address = temp_addr;

        if (DmaSystem_AddItemToQueue(temp_addr, 0x6000+(o->struct_data.npc_data.vram_addr), 128, VRAM_INCHIGH, 1))
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
        if (!DmaSystem_AddItemToQueue(o->struct_data.npc_data.ani.last_address, 0x6000+(o->struct_data.npc_data.vram_addr), 128, VRAM_INCHIGH, 1))
        {
            o->struct_data.npc_data.ani.last_dmafailed = 0;
        }     
    }

    // Only draw every other frame for both visibility and performance
    if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
    {
        uint16_t temp_tileattrib;
        temp_tileattrib = (o->struct_data.npc_data.tilenum | PAL_BUBBLE_E << 9 | 3 << 12);

        SpriteEngine_AddToFrontLayer(o, temp_tileattrib);
    }

    return;
}

void routines_arrow_e(struct game_object * o)
{
    if (!system_game_paused)
    {
        // Move the object based on the stored delta
        move_nocol_fast(o);

        // Check if the object is to be destroyed
        if (o->struct_data.npc_data.ttl == 0)
        {
            obj_destroy_hitbox_enemy(o->array_index);
        }
        else
        {
            // Decrement time to live
            o->struct_data.npc_data.ttl--;
        }
    }

    uint8_t * temp_addr = AniSystem_GetDynamicFrame_Arrow(o);

    bool hflip = false;
    bool vflip = false;

    if (((uint32_t)temp_addr & 0x40000000) == 0x40000000)
    {
        hflip = true;
    }

    if (((uint32_t)temp_addr & 0x80000000) == 0x80000000)
    {
        vflip = true;
    }

    if ((temp_addr != o->struct_data.npc_data.ani.last_address))
    {
        // Save the requested frame into object's data
        // for comparison and in case it fails
        o->struct_data.npc_data.ani.last_address = temp_addr;

        if (DmaSystem_AddItemToQueue(temp_addr, 0x6000+(o->struct_data.npc_data.vram_addr), 128, VRAM_INCHIGH, 1))
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
        if (!DmaSystem_AddItemToQueue(o->struct_data.npc_data.ani.last_address, 0x6000+(o->struct_data.npc_data.vram_addr), 128, VRAM_INCHIGH, 1))
        {
            o->struct_data.npc_data.ani.last_dmafailed = 0;
        }     
    }

    uint16_t temp_tileattrib;
    temp_tileattrib = (o->struct_data.npc_data.tilenum | PAL_LIZARDMAN << 9 | 3 << 12 | hflip << 14 | vflip << 15);

    SpriteEngine_AddToFrontLayer(o, temp_tileattrib);

    return;
}

void routines_hitbox_invis_e(struct game_object * o)
{
    if (system_game_paused)
    {
        return;
    }
    
    // Check if the object is to be destroyed
    if (o->struct_data.npc_data.ttl == 0)
    {
        obj_destroy_hitbox_enemy(o->array_index);
    }
    else
    {
        // Decrement time to live
        o->struct_data.npc_data.ttl--;
    }

    return;
}
