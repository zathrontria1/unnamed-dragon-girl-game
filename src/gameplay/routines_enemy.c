#include <stdlib.h>
#include "snes/console.h"

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

void Routines_Enemy_CommonUpdate(struct game_object * o, const enemy_standard_cfg_t * cfg)
{
    if (!system_game_paused)
    {
        uint16_t temp_invalidate_animation_frame = 0;

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
                
                if (ObjectSystem_IsEnemyAiScheduled(o))
                {
                    temp_invalidate_animation_frame = Routines_Enemy_Ai_Process(o, temp_dist, temp_x, temp_y, cfg->dist_min, cfg->allow_flipflop, cfg->attack_delay, cfg->archetype);
                }

                Routines_Enemy_Ai_UpdateTimer(o);

                int32_t target_dx = o->delta.x.a;
                int32_t target_dy = o->delta.y.a;

                // Move the object based on the stored delta
                if (target_dx != 0 || target_dy != 0)
                {
                    ObjectSystem_Move(o);

                    if ((target_dx != 0 && o->delta.x.a == 0) || (target_dy != 0 && o->delta.y.a == 0))
                    {
                        if (o->struct_data.npc_data.ai_stuck_count < 255)
                        {
                            o->struct_data.npc_data.ai_stuck_count++;
                        }
                    }
                    else
                    {
                        o->struct_data.npc_data.ai_stuck_count = 0;
                    }
                }

                // If the object is attacking:
                if (o->struct_data.npc_data.ai_makeattack)
                {
                    if (cfg->clip_pre_attack != 0)
                    {
                        SoundInterface_PlayClip(cfg->clip_pre_attack);
                    }

                    // Spawn a hit object
                    int16_t j = ObjectSystem_InstantiateEnemyHitbox(cfg->attack_hitbox_id, o->pos.x.lh.h, o->pos.y.lh.h);

                    if (j >= 0)
                    {
                        SoundInterface_PlaySfx_Pre(o, cfg->attack_sfx);

                        struct game_object * hitbox = &obj_hitbox_enemy[j];
                        hitbox->struct_data.npc_data.attack = o->struct_data.npc_data.attack * ENEMY_ATTACK_MULT_RANGED;

                        // Angle needs to be recalculated (worsened accuracy if LOS is missing)
                        uint8_t temp_rand = (uint8_t)Math_GetRandom_u16();
                        uint8_t temp_angle;
                        if (o->struct_data.npc_data.ai_flags & AI_FLAG_LOS)
                        {
                            temp_angle = Math_GetAtan2_u8(temp_y, temp_x) + (temp_rand & 0x0f) - 8;
                        }
                        else
                        {
                            // Blind wild miss penalty (+/- 90 degree spread)
                            temp_angle = Math_GetAtan2_u8(temp_y, temp_x) + (temp_rand & 0x7f) - 64;
                        }

                        o->angle = temp_angle;
                        o->facing = Routines_Enemy_GetFacing(o);

                        if (cfg->set_hitbox_angle)
                        {
                            hitbox->angle = temp_angle;
                        }
                        
                        hitbox->delta.x.a = Math_Cos(temp_angle) * V_MUL * cfg->vel_multiplier;
                        hitbox->delta.y.a = Math_Sin(temp_angle) * V_MUL * cfg->vel_multiplier;

                        hitbox->struct_data.npc_data.ttl = ENEMY_ATTACK_TTL;
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
                            k = ObjectSystem_InstantiateObject(OBJID_SYS_IMPACT, temp_x, temp_y, 0);
                            
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

                                SoundInterface_PlaySfx_Pre(o, SFX_ATK_PUNCH);
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
                    Gfx_EmitSmoke(o, 12);
                }

                if (Routines_Shared_StatusMaintenance(o))
                {
                    temp_invalidate_animation_frame = true;
                }

                // Check if the enemy is dead and set states
                Routines_Shared_CheckIfDead(o);
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

                    k = ObjectSystem_InstantiateObject(OBJID_DROP_MONEY, temp_x, temp_y, 0);

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
                    k = ObjectSystem_InstantiateObject(OBJID_DROP_REC_MEAT, temp_x, temp_y, 0);

                    if (k >= 0)
                    {
                        struct game_object * q = &obj_general[k];

                        q->struct_data.npc_data.hp = ENEMY_DROP_REC_AMOUNT;

                        q->pos.z.a = 0;
                        q->delta.z.a = (4 * V_S_ONE);
                    }
                }

                obj_enemies_defeated++;
                ObjectSystem_DestroyStandardObject(o->array_index);
                return;
            }
        }
        else // spawning
        {
            event_in_combat = 1;
            
            o->struct_data.npc_data.status_time--;
            if (o->struct_data.npc_data.status_time == 0)
            {
                int16_t valid_x = 0;
                int16_t valid_y = 0;
                if (MapSystem_IsPositionSolid(o->pos.x.lh.h, o->pos.y.lh.h, o->w, o->h))
                {
                    if (ObjectSystem_FindValidSpawnPosition(o->pos.x.lh.h, o->pos.y.lh.h, o->w, o->h, &valid_x, &valid_y))
                    {
                        o->pos.x.lh.h = valid_x;
                        o->pos.y.lh.h = valid_y;
                        o->r = valid_x + o->w;
                        o->b = valid_y + o->h;
                        o->state = STATE_IDLE;
                    }
                    else
                    {
                        Routines_Enemy_HandleFailedSpawn(o);
                        ObjectSystem_DestroyStandardObject(o->array_index);
                        return;
                    }
                }
                else
                {
                    o->state = STATE_IDLE;
                }
            }
        }

        // Update every 8 frames
        if (cfg->extra_idle_on_attack_special)
        {
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
}

void Routines_Enemy_Slime(struct game_object * o)
{
    static const enemy_standard_cfg_t cfg = {
        DIST_MELEE, true, 15 / V_MUL, AI_ARCHETYPE_CHASER,
        OBJID_BUBBLE_E, SFX_ATK_SPLASH, 0, 1, false, false
    };

    Routines_Enemy_CommonUpdate(o, &cfg);

    uint8_t * temp_addr = AniSystem_GetDynamicFrame_Slime(o);
    Routines_Shared_Draw(o, temp_addr, PAL_SLIME << 9, 1, false, false);

    return;
}

void Routines_Enemy_Lizardman(struct game_object * o)
{
    static const enemy_standard_cfg_t cfg = {
        DIST_TARGET_RANGE, false, 60 / V_MUL, AI_ARCHETYPE_SKIRMISHER,
        OBJID_ARROW_E, SFX_ATK_SWING, STREAM_HISS, 2, true, true
    };

    Routines_Enemy_CommonUpdate(o, &cfg);

    uint8_t * temp_addr = AniSystem_GetDynamicFrame_Lizardman(o);
    Routines_Shared_Draw(o, temp_addr, PAL_LIZARDMAN << 9, 1, false, false);

    return;
}

void Routines_Enemy_Slime_Bubble(struct game_object * o)
{
    if (!system_game_paused)
    {
        // Move the object based on the stored delta
        ObjectSystem_MoveWithoutCollision(o);

        // Update every 8 frames
        if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8)
        {
            o->struct_data.npc_data.ani.frame ^= 0x0001;
        }

        // Check if the object is to be destroyed
        if (o->struct_data.npc_data.ttl == 0)
        {
            ObjectSystem_DestroyEnemyHitbox(o->array_index);
        }
        else
        {
            // Decrement time to live
            o->struct_data.npc_data.ttl--;
        }
    }

    uint8_t * temp_addr = AniSystem_GetDynamicFrame_Bubble(o);

    Routines_Shared_Draw(o, temp_addr, PAL_BUBBLE_E << 9, 0, true, false);

    return;
}

void Routines_Enemy_Lizardman_Arrow(struct game_object * o)
{
    if (!system_game_paused)
    {
        // Move the object based on the stored delta
        ObjectSystem_MoveWithoutCollision(o);

        // Check if the object is to be destroyed
        if (o->struct_data.npc_data.ttl == 0)
        {
            ObjectSystem_DestroyEnemyHitbox(o->array_index);
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

    uint16_t temp_selected_tile = (((uint32_t)temp_addr >> 24) & 0x3f) << 1;

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

    // Also draw a drop shadow 
    if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
    {
        struct game_object temp;
        temp.pos.x.a = o->pos.x.a;
        temp.pos.y.a = o->pos.y.a + 524288l; // 8 full pixels
        temp.pos.z.a = 0;

        uint16_t temp_tileattrib;
        temp_tileattrib = (0x80 + (temp_selected_tile)) | (PAL_FX_SHADOW << 9) | (2 << 12) | (hflip << 14) | (vflip << 15);

        SpriteEngine_AddToBackLayer(&temp, temp_tileattrib);
    }

    return;
}

void Routines_Enemy_InvisibleHit(struct game_object * o)
{
    if (system_game_paused)
    {
        return;
    }
    
    // Check if the object is to be destroyed
    if (o->struct_data.npc_data.ttl == 0)
    {
        ObjectSystem_DestroyEnemyHitbox(o->array_index);
    }
    else
    {
        // Decrement time to live
        o->struct_data.npc_data.ttl--;
    }

    return;
}

/*
    Adjust the caller's (usually enemy) facing based on angle.

    Player object uses its own way of determining facing
*/
uint16_t Routines_Enemy_GetFacing(struct game_object * o)
{
    return data_facing_lut[(uint8_t)o->angle];
}

/**
 * @brief Handles drop spawning and defeat tracking when an enemy fails to spawn safely.
 * 
 * @param o Pointer to the enemy game object.
 */
void Routines_Enemy_HandleFailedSpawn(struct game_object * o)
{
    int16_t drop_x = o->pos.x.lh.h;
    int16_t drop_y = o->pos.y.lh.h;

    int16_t valid_x = 0;
    int16_t valid_y = 0;
    if (ObjectSystem_FindValidSpawnPosition(drop_x, drop_y, 16, 16, &valid_x, &valid_y))
    {
        drop_x = valid_x;
        drop_y = valid_y;
    }
    else if ((obj_player_pointer != NULL) && ObjectSystem_FindValidSpawnPosition(obj_player_pointer->pos.x.lh.h, obj_player_pointer->pos.y.lh.h, 16, 16, &valid_x, &valid_y))
    {
        drop_x = valid_x;
        drop_y = valid_y;
    }

    int16_t k = -1;
    if (((int16_t)(Math_GetRandom_u16()) > -24576) && (obj_player_recovery_drop_pity != 0))
    {
        obj_player_recovery_drop_pity--;

        k = ObjectSystem_InstantiateObject(OBJID_DROP_MONEY, drop_x, drop_y, 0);

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
        k = ObjectSystem_InstantiateObject(OBJID_DROP_REC_MEAT, drop_x, drop_y, 0);

        if (k >= 0)
        {
            struct game_object * q = &obj_general[k];

            q->struct_data.npc_data.hp = ENEMY_DROP_REC_AMOUNT;

            q->pos.z.a = 0;
            q->delta.z.a = (4 * V_S_ONE);
        }
    }

    obj_enemies_defeated++;
    return;
}
