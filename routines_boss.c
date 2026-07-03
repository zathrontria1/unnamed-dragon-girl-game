#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <snes/console.h>

#include "vars.h"

#include "spr.h"
#include "spr_metaspr.h"

#include "ani.h"
#include "ani_pal.h"

#include "obj.h"
#include "dma.h"

#include "routines.h"
#include "routines_boss.h"

#include "math_int.h"

#include "ui.h"

#include "snd.h"
#include "consts_snd.h"

#include "hittest.h"

#include "gfx.h"

#include "movement.h"

#include "main.h"

#include "system.h"
#include "map.h"

uint16_t obj_boss_state; // Boss state machine variable
bool obj_boss_palette_swap;

int obj_boss_phase;
int obj_boss_subphase;

int obj_boss_timer_movement;
bool obj_boss_moving;

int obj_boss_timer_attack;

uint16_t obj_boss_prev_frame;
bool obj_boss_vram_stale;

#define BOSS_ATTACK_BASETIME_1 ((5 * FPS) >> 1) - 1

void Routines_Boss_Test(struct game_object * o)
{
    struct game_object * p = obj_player_pointer;
    
    bool temp_facing_left = false;

    int z_offset = (system_frames_elapsed & 0x1f) >> 2;
    if (z_offset > 3)
    {
        z_offset = 7-z_offset;
    }

    o->pos.z.lh.h = z_offset;

    if (p->pos.x.lh.h < (o->pos.x.lh.h + (o->w >> 1)))
    {
        temp_facing_left = true;
    }

    if (!system_game_paused)
    {
        bool temp_invalidate_animation_frame = false;

        if ((o->state != STATE_DIE) && (o->state != STATE_SPAWNING))
        {
            // Place boss logic here
            event_in_combat = 1;

            temp_invalidate_animation_frame |= Routines_Boss_Test_RunPhase(o);
            temp_invalidate_animation_frame |= Routines_Boss_Test_TestAgainstHits(o);

            if (o->struct_data.npc_data.status == STATUS_BURNING)
            {
                // Burning boss produce vfx
                Gfx_EmitSmoke(o, 64);

                // Burning boss palette cycles
                if (!obj_boss_palette_swap)
                {
                    AniSystem_Pal_LoadCycleSubpalette((uint8_t *)&data_palette_cycle_fire, 0);
                    obj_boss_palette_swap = true;
                }

                if (system_frames_elapsed & 0x01)
                {
                    AniSystem_Pal_CycleSubpalette(0);
                    AniSystem_Pal_CopyCycledSubpaletteToMainSubpalette(0, PAL_BOSS_TEST+8, 0, 8);
                }
            }
            else if (o->struct_data.npc_data.status == STATUS_NORMAL)
            {
                if (obj_boss_palette_swap)
                {
                    AniSystem_Pal_LoadSubpalette((uint8_t *)&data_palette_boss_0, PAL_BOSS_TEST+8);
                    obj_boss_palette_swap = false;
                }
            }

            if (Routines_Shared_StatusMaintenance(o))
            {
                temp_invalidate_animation_frame = true;
            }

            // Check if the enemy is dead and set states
            Routines_Shared_CheckIfDead(o);
        }
        else if (o->state == STATE_DIE)
        {
            // Place events to take place when the boss dies here
            ;
        }
        else // spawning
        {
            // Delay when a boss is spawning
            event_in_combat = 1;
            
            o->struct_data.npc_data.status_time--;
            if (o->struct_data.npc_data.status_time == 0)
            {
                o->state = STATE_IDLE;
            }
        }
    }

    if ((system_frames_elapsed & 0x03) == 0x00)
    {
        o->struct_data.npc_data.ani.frame = ((o->struct_data.npc_data.ani.frame + 1) & 0x07); // 8 frame animation
    }

    Routines_Boss_Test_ReconstructFrame(o);
    Routines_Boss_Test_DmaFrame(o);

    if (temp_facing_left)
    {
        SpriteEngine_AddMetaSprite(o, (const struct spr_metaspr_definition *)&data_metaspr_boss_generic_64x96_hflip);
    }
    else
    {
        SpriteEngine_AddMetaSprite(o, (const struct spr_metaspr_definition *)&data_metaspr_boss_generic_64x96);
    }

    if (system_frames_elapsed & 0x01)
    {
        Routines_Boss_Test_DrawShadow(o);
    }

    return;
}

/*
    Reconstruct boss frame based on the lookup table

    They're basically 16x16 slices made from 16x8 slices
*/
void Routines_Boss_Test_ReconstructFrame(struct game_object * o)
{
    uint16_t frame_offset = o->struct_data.npc_data.ani.frame;

    Routines_Boss_Test_GetCompressedFrame((const uint8_t *)&data_spr_boss_placeholder_dd, (const uint16_t *)&data_spr_boss_placeholder_lut, (uint8_t *)(0x007fe000), frame_offset);

    return;
}

void Routines_Boss_Test_DmaFrame(struct game_object * o)
{
    if (obj_boss_vram_stale)
    {
        DmaSystem_AddItemToQueue((uint8_t *)(0x007fe000), 0x7000, 3072, VRAM_INCHIGH, 0);
        obj_boss_vram_stale = false;
    }

    return;
}

/*
    Routine to rebuild boss graphics data from 48 DMA runs
*/
uint8_t * Routines_Boss_Test_GetCompressedFrame(const uint8_t * data, const uint16_t * lookup, uint8_t * buffer, uint16_t frame)
{
    uint16_t lookup_entry_offset = frame * 48; // Each frame is 96 bytes. Use halved values

    uint8_t * ptr_return_val = (uint8_t *)(lookup + lookup_entry_offset);

    if (frame == obj_boss_prev_frame)
    {
        return ptr_return_val;
    }

    obj_boss_prev_frame = frame;

    uint16_t * data_offset = (uint16_t *)ptr_return_val;

    uint16_t data_addr_lo = (uint16_t)((uint32_t)(data));

    uint16_t ptr_array[48];

    for (int i = 0; i < 48; i++)
    {
        ptr_array[i] = data_addr_lo + *data_offset++;
    }

    // Use the DMA unit to speed things up. Channel 7 is reserved for active display DMA.
    // Align read
    REG_DMAP7 = 0x00; // byte reg write
    REG_BBAD7 = 0x80; // WMDATA
    
    REG_WMADDH = (uint8_t)((uint32_t)buffer >> 16);

    REG_A1B7 = (uint8_t)((uint32_t)data >> 16);

    int i = 0;

    for (int s = 0; s < 2; s++)
    {
        System_Hsync(156); // This is enough
        for (int y = 0; y < 6; y++)
        {
            REG_WMADDLM = (uint16_t)((uint32_t)buffer + (s << 8) + (y << 9));

            for (int x = 0; x < 4; x++)
            {
                REG_A1T7LH = ptr_array[i];
                REG_DAS7LH = 64;
                REG_MDMAEN = 0x80;

                i++;
            }
        }
    }

    obj_boss_vram_stale = true;

    return ptr_return_val;
}

/*
    Draws a very large shadow
*/
void Routines_Boss_Test_DrawShadow(struct game_object * o)
{
    struct game_object temp;
    temp.pos.x.a = o->pos.x.a;
    temp.pos.y.a = o->pos.y.a;
    temp.pos.z.a = 0;

    SpriteEngine_AddMetaSprite_Back(&temp, (const struct spr_metaspr_definition *)&data_metaspr_shadow_64x16);

    return;
}

bool Routines_Boss_Test_TestAgainstHits(struct game_object * o)
{
    bool temp_invalidate_animation_frame = false;

    if (o->struct_data.npc_data.invuln_time == 0)
    {
        if ((o->uid & 0x0001) == ((unsigned int)system_frames_elapsed & 0x0001))
        {
            struct game_object * p = CollisionCheck_EnemyTestPlayer(o);
            if (p != NULL)
            {
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

                temp_invalidate_animation_frame = true;

                o->struct_data.npc_data.hp_display_time = 60 / V_MUL;
            }
        }
    }

    return temp_invalidate_animation_frame;
}

/*
    Phase based processing

    For now there's only one phase
*/
bool Routines_Boss_Test_RunPhase(struct game_object * o)
{
    bool temp_invalidate_animation_frame = false;

    // Movement logic.

    if (obj_boss_timer_movement != 0)
    {
        obj_boss_timer_movement--;

        if (!obj_boss_timer_movement)
        {
            obj_boss_moving = false;
        }
    }

    if (!obj_boss_moving)
    {
        // Pick a spot to move to.
        uint16_t select = (Math_GetRandom_u16() % 9) << 1;

        int32_t dest_x = (int32_t)const_boss_positions_0[select] << 16;
        int32_t dest_y = (int32_t)const_boss_positions_0[select+1] << 16;

        temp_invalidate_animation_frame |= Routines_Boss_Test_Movement(o, dest_x, dest_y);

        obj_boss_moving = true;
    }

    // Attack logic.
    if (obj_boss_timer_attack != 0)
    {
        obj_boss_timer_attack--;
    }

    if (!obj_boss_timer_attack)
    {
        int32_t dest_x;
        int32_t dest_y;

        // Pick a spot to attack.
        switch (obj_boss_phase)
        {
            case 0:
                dest_x = 400;
                dest_y = 368;
                break;

            case 1:
                dest_x = 528;
                dest_y = 368;
                break;

            case 2:
                dest_x = 528;
                dest_y = 496;
                break;

            case 3:
                dest_x = 400;
                dest_y = 496;
                break;
        }

        temp_invalidate_animation_frame |= Routines_Boss_Test_Attack_Pattern1(o, dest_x, dest_y);

        if (!obj_boss_subphase)
        {
            obj_boss_phase = (obj_boss_phase + 1) & 0x03;
        }
    }

    // Finalize the movement.
    ObjectSystem_Move(o);

    return temp_invalidate_animation_frame;
}

/*
    Called to perform an attack pattern

    TODO: relevant graphics and supporting subroutine for the attack objects.
*/
bool Routines_Boss_Test_Attack_Pattern1(struct game_object * o, int32_t x, int32_t y)
{
    bool temp_invalidate_animation_frame = false;

    switch (obj_boss_phase)
    {
        case 0:
            x = x + (obj_boss_subphase << 3);
            break;
        case 1:
            y = y + (obj_boss_subphase << 3);
            break;
        case 2:
            x = x - (obj_boss_subphase << 3);
            break;
        case 3:
            y = y - (obj_boss_subphase << 3);
            break;
    }

    int j = ObjectSystem_InstantiateEnemyHitbox(OBJID_BOSS_TEST1_ATTACK1, x, y);

    if (j >= 0)
    {
        struct game_object * p = &obj_hitbox_enemy[j];
        //p->struct_data.npc_data.attack = o->struct_data.npc_data.attack * ENEMY_ATTACK_MULT_RANGED;
        p->struct_data.npc_data.attack = 1;
        
        switch (obj_boss_phase)
        {
            case 0:
                p->delta.x.a = 0;
                p->delta.y.a = V_S_ONE;
                break;
            case 1:
                p->delta.x.a = -V_S_ONE;
                p->delta.y.a = 0;
                break;
            case 2:
                p->delta.x.a = 0;
                p->delta.y.a = -V_S_ONE;
                break;
            case 3:
                p->delta.x.a = V_S_ONE;
                p->delta.y.a = 0;
                break;
        }

        p->struct_data.npc_data.ttl = BOSS_ATTACK_BASETIME_1;
    }

    obj_boss_timer_attack = (5 * FPS) >> 4;

    obj_boss_subphase = (obj_boss_subphase + 1) & 0x0f;

    return temp_invalidate_animation_frame;
}

/*
    Subroutine for the object
*/
void Routines_Boss_Test_Attack_Particle(struct game_object * o)
{
    if (!system_game_paused)
    {
        int z_offset = (system_frames_elapsed & 0x1f) >> 2;
        if (z_offset > 3)
        {
            z_offset = 7-z_offset;
        }
        
        o->pos.z.lh.h = z_offset + 4;

        // Move the object based on the stored delta
        ObjectSystem_MoveWithoutCollision(o);

        if (o->struct_data.npc_data.ttl > BOSS_ATTACK_BASETIME_1 - 6)
        {
            o->struct_data.npc_data.ani.frame = 7 - (BOSS_ATTACK_BASETIME_1 - o->struct_data.npc_data.ttl);
        }
        else if (o->struct_data.npc_data.ttl > 6)
        {
            // Update every 8 frames
            if (!((uint16_t)system_frames_elapsed & ANI_INTERVAL_8))
            {
                o->struct_data.npc_data.ani.frame = (o->struct_data.npc_data.ani.frame ^ 0x0001) & 0x0001;
            }
        }   
        else
        {
            o->struct_data.npc_data.ani.frame = 7 - o->struct_data.npc_data.ttl;
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

    uint8_t * temp_addr = AniSystem_GetDynamicFrame_EnemyBossParticle(o);

    Routines_Shared_Draw(o, temp_addr, PAL_BOSS_TEST, 1, false, false);

    AniSystem_DrawDropShadow(o);

    return;
}

/*
    Called to calculate the correct movement delta to finish moving to target position
*/
bool Routines_Boss_Test_Movement(struct game_object * o, int32_t x, int32_t y)
{
    bool temp_invalidate_animation_frame = false;

    // Get difference between target location and current position
    int32_t diff_x = x - o->pos.x.a;
    int32_t diff_y = y - o->pos.y.a;

    // Now calculate the correct object delta
    int32_t delta_x = diff_x / (5 * FPS);
    int32_t delta_y = diff_y / (5 * FPS);

    // Set the delta
    o->delta.x.a = delta_x;
    o->delta.y.a = delta_y;

    obj_boss_timer_movement = 5 * FPS; // Move for 5 seconds.

    return temp_invalidate_animation_frame;
}

/*
    Table of coordinates that the boss can move to
    in X/Y order, 16-bits
    game uses 32 bits so must expand with << 16
*/
const int16_t const_boss_positions_0[] = 
{
    384, 384,
    456, 384,
    528, 384,

    384, 456,
    456, 456,
    528, 456,

    384, 528,
    456, 528,
    528, 528,
};
