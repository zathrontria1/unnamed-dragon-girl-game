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

            if (o->struct_data.npc_data.status == STATUS_BURNING)
            {
                if (!obj_boss_palette_swap)
                {
                    AniSystem_Pal_LoadSubpalette((uint8_t *)&data_palette_cycle_fire, PAL_BOSS_TEST+8);
                    obj_boss_palette_swap = true;
                }

                if (system_frames_elapsed & 0x01)
                {
                    AniSystem_Pal_CycleSubpalette(PAL_BOSS_TEST+8);
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
    DmaSystem_AddItemToQueue((uint8_t *)(0x007fe000), 0x7000, 3072, VRAM_INCHIGH, 0);

    return;
}

uint8_t * Routines_Boss_Test_GetCompressedFrame(const uint8_t * data, const uint16_t * lookup, uint8_t * buffer, uint16_t frame)
{
    uint16_t lookup_entry_offset = frame * 48; // Each frame is 96 bytes. Use halved values

    uint8_t * ptr_return_val = (uint8_t *)(lookup + lookup_entry_offset);

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