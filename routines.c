#include <stdio.h>
#include <stdlib.h>
#include <snes/console.h>

#include "vars.h"

#include "spr.h"
#include "spr_metaspr.h"

#include "ani.h"
#include "ani_fixedspr.h"

#include "obj.h"
#include "dma.h"
#include "routines.h"
#include "routines_player.h"
#include "routines_enemy.h"
#include "math_int.h"
#include "map.h"
#include "system.h"

#include "ui.h"
#include "ui_messagebox.h"
#include "ui_vwf.h"

#include "data_strings.h"

#include "snd.h"
#include "consts_snd.h"

#include "hittest.h"

#include "gfx.h"

#include "movement.h"

#include "main.h"

#include "level.h"

void Routines_Fx_Smoke(struct game_object * o)
{
    if (!system_game_paused)
    {
        // Move the object based on the stored delta
        ObjectSystem_MoveWithoutCollision_Fast(o);

        // Update every 8 frames
        if (!((uint16_t)system_frames_elapsed & ANI_INTERVAL_8))
        {
            o->struct_data.npc_data.ani.frame ^= 0x0001;
        }

        // Check if the object is to be destroyed
        if (!o->struct_data.npc_data.ttl)
        {
            ObjectSystem_DestroyStandardObject(o->array_index);
        }
        else
        {
            // Decrement time to live
            o->struct_data.npc_data.ttl--;
        }
    }

    unsigned int offset = o->struct_data.npc_data.ani.frame << 1;

    Routines_Shared_DrawFixed(o, (0x0006 + offset) | PAL_FX_SMOKE << 9, 0, true); 

    return;
}

void Routines_Fx_Impact(struct game_object * o)
{
    Routines_Shared_DrawFixed(o, 0x000a | PAL_FX_IMPACT << 9, 0, true);

    if (system_game_paused)
    {
        return;
    }

    // Check if the object is to be destroyed
    if (!o->struct_data.npc_data.ttl)
    {
        ObjectSystem_DestroyStandardObject(o->array_index);
    }
    else
    {
        // Decrement time to live
        o->struct_data.npc_data.ttl--;
    }

    return;
}

void Routines_Interactables_Switch(struct game_object * o)
{
    if (!system_game_paused)
    {
        // Check if a player hit is on the switch

        // Only test when the switch can react (i.e. after timeout)
        // And while not in combat
        if (!o->struct_data.interactable_data.delay_time)
        {
            if (CollisionCheck_InteractableTestPlayerAction(o))
            {
                if (!event_in_combat_shadow)
                {
                    SoundInterface_PlaySfx_Pre(o, SFX_INTERACT_SWITCH);
                    
                    o->state ^= STATE_SWITCH_ON;
                    event_flags_local[o->struct_data.interactable_data.event_flag] = o->state;
                    o->struct_data.interactable_data.delay_time = 15 / V_MUL; // responsive enough for punch clicks

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
                }
                else
                {
                    UserInterface_PrintText((uint8_t *)&STR_MSG_INCOMBAT, UI_MSGBOX_SL_START, UI_MARGIN_LEFT);
                }
            }
        }
        else
        {
            o->struct_data.interactable_data.delay_time--;
        }
    }

    Routines_Shared_DrawFixed(o, 0x20 + (o->state << 1) | PAL_INTERACTABLE_SWITCH_WALL << 9, 2, false);

    return;
}

void Routines_Interactable_Sign(struct game_object * o)
{
    if (!system_game_paused)
    {
        // Check if a player hit is on the sign
        // And while not in combat
        if (o->struct_data.interactable_data.delay_time == 0)
        {
            if (CollisionCheck_InteractableTestPlayerAction(o) != 0)
            {
                if (!event_in_combat_shadow)
                {
                    SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);

                    UserInterface_DrawTextbox(UI_MSGBOX_ML_START, UI_MSGBOX_HEIGHT);

                    VwfEngine_PrintText_Gradual_Setup((uint8_t *)o->data_ptr, (uint8_t *)(LZ4_BUFFER_ADDR+0x8000), (uint8_t *)(LZ4_BUFFER_ADDR+0x8800), UI_MARGIN_LEFT, 0, 0x80, 128);
                    DmaSystem_AddItemToQueue((uint8_t *)(LZ4_BUFFER_ADDR+0x8000), 0x4400, 16, VRAM_INCHIGH, 0);
                    DmaSystem_AddItemToQueue((uint8_t *)(LZ4_BUFFER_ADDR+0x8800), TILEMAP_ADDR_GAME_UI_2BPP+((UI_MSGBOX_ML_START + 1) << 5), 256, VRAM_INCHIGH, 0);
                    
                    system_loop_func_ptr = Main_GetFunctionPointer(ROUTINE_MSGBOX);
                    //system_current_routine = ROUTINE_MSGBOX;
                    system_target_routine = ROUTINE_MSGBOX;

                    o->struct_data.interactable_data.delay_time = 15 / V_MUL; // responsive enough for punch clicks
                }
                else
                {
                    UserInterface_PrintText((uint8_t *)&STR_MSG_INCOMBAT, UI_MSGBOX_SL_START, UI_MARGIN_LEFT);
                }
            }
        }
        else
        {
            o->struct_data.interactable_data.delay_time--;
        }
    }

    Routines_Shared_DrawFixed(o, 0x28 | PAL_INTERACTABLE_SIGN_WALL << 9, 2, false);

    return;
}

void Routines_TreasureChest(struct game_object * o)
{
    if (!system_game_paused)
    {
        if (!o->struct_data.interactable_data.opened)
        {
            // Check if a player hit is on the sign
            // And while not in combat
            if (CollisionCheck_InteractableTestPlayerAction(o) != 0)
            {
                if (!event_in_combat_shadow)
                {
                    SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);

                    obj_player_pointer->struct_data.npc_data.money += o->struct_data.interactable_data.money;
                    
                    o->struct_data.interactable_data.opened = true;

                    char temp_str[32] = "";
                    snprintf((char *)&temp_str, 32, (char *)&STR_MSG_FOUNDMONEY, (int32_t)o->struct_data.interactable_data.money);
                    UserInterface_PrintText((char *)&temp_str, UI_MSGBOX_SL_START, UI_MARGIN_LEFT);

                    o->struct_data.interactable_data.ttl = 180 / V_MUL;

                    //SoundInterface_PlayClip(STREAM_VOICE_TREASURE_1 + (Math_GetRandom_u16() & 0x01));
                }
                else
                {
                    UserInterface_PrintText((uint8_t *)&STR_MSG_INCOMBAT, UI_MSGBOX_SL_START, UI_MARGIN_LEFT);
                }
            }
        }
        else
        {
            // Check if the object is to be destroyed
            if (o->struct_data.interactable_data.ttl == 0)
            {
                ObjectSystem_DestroyStandardObject(o->array_index);
            }
            else
            {
                // Decrement time to live
                o->struct_data.interactable_data.ttl--;
            }
        }
    }

    unsigned int offset = (o->struct_data.interactable_data.opened << 1);
    Routines_Shared_DrawFixed(o, 0x24+(offset) | PAL_INTERACTABLE_TREASURECHEST << 9, 1, false);

    return;
}

void Routines_Interactable_Blocker(struct game_object * o)
{
    if (!system_game_paused) // If game is not paused
    {
        bool temp_switch_flip = false;

        if (o->state != event_flags_local[o->struct_data.interactable_data.event_flag])
        {
            temp_switch_flip = true;
        }
        
        if (o->struct_data.interactable_data.delay_time != 0) // If the blocker is run for the first time
        {
            o->struct_data.interactable_data.delay_time = 0;
            temp_switch_flip = true;
        }
        
        o->state = event_flags_local[o->struct_data.interactable_data.event_flag];

        if (temp_switch_flip == true)
        {
            if (o->state == STATE_SWITCH_OFF)
            {
                // edit the map_collision_buf
                // o->tile contains the x and y tile coords
                // y needs to be shifted an amount of times
                uint16_t q = (o->tile.y << map_extent_tiles_x_shiftcount) + o->tile.x;
                uint16_t q2;
                map_collision_buf[q] = 0x00;

                switch (o->id)
                {
                    case OBJID_INTERACTABLE_BLOCKER_DOOR_NS:
                        q2 = (o->tile.y << map_extent_tiles_x_shiftcount) + o->tile.x + 1;
                        map_collision_buf[q2] = 0x00;
                        break;
                    case OBJID_INTERACTABLE_BLOCKER_DOOR_EW:
                        q2 = ((o->tile.y - 1) << map_extent_tiles_x_shiftcount) + o->tile.x;
                        map_collision_buf[q2] = 0x00;
                        break;
                }
            }
            else
            {
                // edit the map_collision_buf
                // o->tile contains the x and y tile coords
                // y needs to be shifted an amount of times
                uint16_t q = (o->tile.y << map_extent_tiles_x_shiftcount) + o->tile.x;
                uint16_t q2;
                map_collision_buf[q] = 0xff;

                switch (o->id)
                {
                    case OBJID_INTERACTABLE_BLOCKER_DOOR_NS:
                        q2 = (o->tile.y << map_extent_tiles_x_shiftcount) + o->tile.x + 1;
                        map_collision_buf[q2] = 0xff;
                        break;
                    case OBJID_INTERACTABLE_BLOCKER_DOOR_EW:
                        q2 = ((o->tile.y - 1) << map_extent_tiles_x_shiftcount) + o->tile.x;
                        map_collision_buf[q2] = 0xff;
                        break;
                }
            }
        }
    }

    if (o->state == STATE_SWITCH_OFF) 
    {
        switch (o->id)
        {
            case OBJID_INTERACTABLE_BLOCKER_DOOR_NS:
                SpriteEngine_AddMetaSprite(o, &data_metaspr_door_ns_closed[0]);
                break;
            case OBJID_INTERACTABLE_BLOCKER_DOOR_EW:
                SpriteEngine_AddMetaSprite(o, &data_metaspr_door_ew_closed[0]);
                break;
        }
    }
    else
    {
        switch (o->id)
        {
            case OBJID_INTERACTABLE_BLOCKER_DOOR_NS:
                SpriteEngine_AddMetaSprite(o, &data_metaspr_door_ns_open[0]);
                break;
            case OBJID_INTERACTABLE_BLOCKER_DOOR_EW:
                if (system_frames_elapsed & 0x01)
                {
                    SpriteEngine_AddMetaSprite(o, &data_metaspr_door_ew_open_flip[0]);
                }
                else
                {
                    SpriteEngine_AddMetaSprite(o, &data_metaspr_door_ew_open[0]);
                }
                break;
        }
    }

    return;
}

void Routines_LevelWarp(struct game_object * o)
{
    bool warp_open = false;
    if (!system_game_paused) // If game is not paused
    {
        if (obj_enemies_defeated >= obj_enemies_target_count)
        {
            // edit the map_collision_buf
            // o->tile contains the x and y tile coords
            // y needs to be shifted an amount of times
            uint16_t q = (o->tile.y << map_extent_tiles_x_shiftcount) + o->tile.x;
            uint16_t q2;
            uint16_t q3;
            uint16_t q4;
            map_collision_buf[q] = 0xff;

            q2 = (o->tile.y << map_extent_tiles_x_shiftcount) + o->tile.x + 1;
            map_collision_buf[q2] = 0xff;

            q3 = ((o->tile.y - 1) << map_extent_tiles_x_shiftcount) + o->tile.x;
            map_collision_buf[q3] = 0xff;

            q4 = ((o->tile.y - 1) << map_extent_tiles_x_shiftcount) + o->tile.x + 1;
            map_collision_buf[q4] = 0xff;

            warp_open = true;
        }
    }

    if (!warp_open)
    {
        SpriteEngine_AddMetaSprite(o, &data_metaspr_level_warp_closed[0]);
    }
    else
    {
        // Test if the player is inside the warp area here.
        // Can use same test as the drops, just use a larger area
        struct game_object * p = obj_player_pointer;

        int16_t x1 = p->pos.x.lh.h;
        int16_t x2 = o->pos.x.lh.h;

        int16_t y1 = p->pos.y.lh.h;
        int16_t y2 = o->pos.y.lh.h - 16;

        int16_t w1 = p->w;
        int16_t w2 = 32;

        int16_t h1 = p->h;
        int16_t h2 = 16;

        // Check if the player is within the designated box
        if (CollisionCheck_Aabb_Direct_Rectangle(x1, x2, y1, y2, w1, w2, h1, h2) == 0)
        {
            level_data_ptr_next = (const struct level_data *)o->data_ptr;
        }
    }

    return;
}

void Routines_EnemySpawner(struct game_object * o)
{
    if (!system_game_paused)
    {
        int16_t x1 = obj_player_pointer->pos.x.lh.h;
        int16_t y1 = obj_player_pointer->pos.y.lh.h;

        // Check if the player is within the designated box
        if (CollisionCheck_Aabb_Direct_Rectangle(x1, o->struct_data.interactable_data.spawn_area_x, y1, o->struct_data.interactable_data.spawn_area_y, 16, o->struct_data.interactable_data.spawn_area_w, 16, o->struct_data.interactable_data.spawn_area_h) == 0)
        {
            ObjectSystem_List_InstantiateNpcs((struct obj_list_entry_spawns *)o->data_ptr, o->pos.x.lh.h, o->pos.y.lh.h);

            // Set the camera bounds
            bg_scroll_x_bounds_min.full.high.a = o->struct_data.interactable_data.screen_x;
            bg_scroll_y_bounds_min.full.high.a = o->struct_data.interactable_data.screen_y;

            bg_scroll_x_bounds_max.full.high.a = (o->struct_data.interactable_data.screen_x + o->struct_data.interactable_data.screen_w) - 256;
            bg_scroll_y_bounds_max.full.high.a = (o->struct_data.interactable_data.screen_y + o->struct_data.interactable_data.screen_h) - 224;

            if (bg_scroll_x_bounds_max.full.high.a < bg_scroll_x_bounds_min.full.high.a)
            {
                bg_scroll_x_bounds_max.full.high.a = bg_scroll_x_bounds_min.full.high.a;
            }
            if (bg_scroll_y_bounds_max.full.high.a < bg_scroll_y_bounds_min.full.high.a)
            {
                bg_scroll_y_bounds_max.full.high.a = bg_scroll_y_bounds_min.full.high.a;
            }

            bg_scroll_use_interpolation = true;
            bg_scroll_suppress_interpolation_state_change = 2; // intentional

            ObjectSystem_DestroyStandardObject(o->array_index);
        }
    }

    return;
}

void Routines_Drops_Money(struct game_object * o)
{
    if (!system_game_paused)
    {
        if (AniSystem_AnimateDropGravity(o))
        {
            SoundInterface_PlaySfx_Pre(o, SFX_DROP_COIN);
        }

        if (o->pos.z.a == 0) // If item is on floor
        {
            struct game_object * p = obj_player_pointer;

            // Check if the player is within the designated box
            if (CollisionCheck_Aabb_BetweenObjects(p, o) == 0)
            {
                SoundInterface_PlaySfx_Pre(o, SFX_DROP_COIN);

                p->struct_data.npc_data.money += o->struct_data.npc_data.money;

                ObjectSystem_DestroyStandardObject(o->array_index);
            }
        }
    }

    Routines_Shared_DrawFixed(o, 0x2a | PAL_DROP_MONEY << 9, 1, false);

    return;
}

void Routines_Drops_Recovery_Meat(struct game_object * o)
{
    if (!system_game_paused)
    {
        if (AniSystem_AnimateDropGravity(o))
        {
            SoundInterface_PlaySfx_Pre(o, SFX_DROP_BOUNCE);
        }

        if (o->pos.z.a == 0) // If item is on floor
        {
            struct game_object * p = obj_player_pointer;
            
            // Check if the player is within the designated box
            if (CollisionCheck_Aabb_BetweenObjects(p, o) == 0)
            {
                SoundInterface_PlaySfx_Pre(o, SFX_DROP_BOUNCE);
                
                if (p->struct_data.npc_data.hp + o->struct_data.npc_data.hp >= p->struct_data.npc_data.hp_max)
                {
                    p->struct_data.npc_data.hp = p->struct_data.npc_data.hp_max;
                }
                else
                {
                    p->struct_data.npc_data.hp += o->struct_data.npc_data.hp;
                }

                Gfx_SetColorMath(0,4,0);
                gfx_cmath_change = -64 * V_MUL;
                shadow_cgwsub = 0x00;
                shadow_cgadsub = 0x32;
                
                ObjectSystem_DestroyStandardObject(o->array_index);
            }
        }
    }

    Routines_Shared_DrawFixed(o, 0xa0 | PAL_DROP_REC_MEAT << 9, 1, false);

    return;
}

void Routines_Dummy(struct game_object * o)
{
    return;
}

/*
    Called to decrement the same things used by many objects

    Returns true if frame needs invalidation
*/
bool Routines_Shared_StatusMaintenance(struct game_object * o)
{
    bool invalidate = false;

    if (o->struct_data.npc_data.invuln_time > 0)
    {
        o->struct_data.npc_data.invuln_time--;
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

            invalidate = 1;
        }
    }

    return invalidate;
}

/*
    Called to check if object has died and if it has automatically set
    variables
*/
void Routines_Shared_CheckIfDead(struct game_object * o)
{
    if ((o->struct_data.npc_data.hp <= 0) && (o->state != STATE_DIE))
    {
        o->state = STATE_DIE;
        o->struct_data.npc_data.ani.frame = 0;
        o->struct_data.npc_data.status_time = 64 / V_MUL;
    }

    return;
}


/*
    Used to draw an entity after getting the appropriate attributes and pointers.

    The sign bit in the sprite address pointer determines horizontal flipping.
*/
void Routines_Shared_Draw(struct game_object * o, uint8_t * spr_addr, int pal, int layer, bool always_flicker, bool is_player)
{
    if ((spr_addr != o->struct_data.npc_data.ani.last_address))
    {
        if (is_player)
        {
            if (DmaSystem_AddItemToQueue((uint8_t *)(LZ4_BUFFER_ADDR+0xc000), 0x6000, 128, VRAM_INCHIGH, 1))
            {
                o->struct_data.npc_data.ani.last_dmafailed = 1;
            }
            else
            {
                o->struct_data.npc_data.ani.last_dmafailed = 0;
            }
        }
        else
        {
            if (DmaSystem_AddItemToQueue(spr_addr, 0x6000+(o->struct_data.npc_data.vram_addr), 128, VRAM_INCHIGH, 1))
            {
                o->struct_data.npc_data.ani.last_dmafailed = 1;
            }
            else
            {
                o->struct_data.npc_data.ani.last_dmafailed = 0;
            }
        }
        // Save the requested frame into object's data
        // for comparison and in case it fails
        o->struct_data.npc_data.ani.last_address = spr_addr; 
    }
    else if ((o->struct_data.npc_data.ani.last_dmafailed))
    {
        // The previous DMA failed. Attempt it again.
        if (is_player)
        {
            if (!(DmaSystem_AddItemToQueue((uint8_t *)(LZ4_BUFFER_ADDR+0xc000), 0x6000, 128, VRAM_INCHIGH, 1)))
            {
                o->struct_data.npc_data.ani.last_dmafailed = 0;
            }   
        }
        else
        {
            if (!DmaSystem_AddItemToQueue(o->struct_data.npc_data.ani.last_address, 0x6000+(o->struct_data.npc_data.vram_addr), 128, VRAM_INCHIGH, 1))
            {
                o->struct_data.npc_data.ani.last_dmafailed = 0;
            }   
        } 
    }

    // DMA variant
    uint16_t tileattrib = o->struct_data.npc_data.tilenum | pal << 9;

    if (((uint32_t)spr_addr & 0x80000000) == 0x80000000) // sign bit is used for flip
    {
        tileattrib |= true << 14;
    }

    bool flicker_pass = false;

    if (!always_flicker) 
    {
        if (!(shadow_stat77 & 0x80))
        {
            // No overflow, always draw
            flicker_pass = true;
        }
    }

    if (!flicker_pass)
    {
        // Flicker didn't auto-pass. Test against current frame odd/even.
        if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
        {
            flicker_pass = true;
        } 
    }

    // Only draw sprites not flickered away
    if (flicker_pass)
    {
        if (layer == 0)
        {
            tileattrib |= 3 << 12;

            SpriteEngine_AddToFrontLayer(o, tileattrib);
        }
        else if (layer == 1)
        {
            tileattrib |= 2 << 12;

            SpriteEngine_AddToSortedLayer(o, tileattrib);
        }
        else
        {
            tileattrib |= 2 << 12;

            SpriteEngine_AddToBackLayer(o, tileattrib);
        }
    }

    return;
}

/*
    Draws a fixed sprite from the sprite pages.

    Tileattrib must contain the tilenum, palette, and h/v flip information. Priority is not needed.
*/
void Routines_Shared_DrawFixed(struct game_object * o, uint16_t tileattrib, int layer, bool always_flicker)
{
    bool flicker_pass = false;

    if (!always_flicker) 
    {
        if (!(shadow_stat77 & 0x80))
        {
            // No overflow, always draw
            flicker_pass = true;
        }
    }

    if (!flicker_pass)
    {
        // Flicker didn't auto-pass. Test against current frame odd/even.
        if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
        {
            flicker_pass = true;
        } 
    }

    // Only draw sprites not flickered away
    if (flicker_pass)
    {
        if (layer == 0)
        {
            tileattrib |= 3 << 12;

            SpriteEngine_AddToFrontLayer(o, tileattrib);
        }
        else if (layer == 1)
        {
            tileattrib |= 2 << 12;

            SpriteEngine_AddToSortedLayer(o, tileattrib);
        }
        else
        {
            tileattrib |= 2 << 12;

            SpriteEngine_AddToBackLayer(o, tileattrib);
        }
    }

    return;
}