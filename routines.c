#include <stdio.h>
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
#include "routines_enemy.h"
#include "math_int.h"
#include "map.h"
#include "system.h"

#include "ui.h"
#include "ui_messagebox.h"

#include "data_strings.h"

#include "snd.h"
#include "consts_snd.h"

#include "hittest.h"

#include "gfx.h"

#include "movement.h"

#include "main.h"

void routines_fx_smoke(struct game_object * o)
{
    if (!system_game_paused)
    {
        // Move the object based on the stored delta
        move_nocol_veryfast(o);

        // Update every 8 frames
        if (((uint16_t)system_frames_elapsed & ANI_INTERVAL_8) == ANI_INTERVAL_8)
        {
            o->struct_data.npc_data.ani.frame ^= 0x0001;

            o->struct_data.npc_data.ani.display = AniSystem_GetFixedFrame_Fast(o);
        }

        // Check if the object is to be destroyed
        if (o->struct_data.npc_data.ttl == 0)
        {
            obj_destroy(o->array_index);
        }
        else
        {
            // Decrement time to live
            o->struct_data.npc_data.ttl--;
        }
    }

    uint16_t temp_tileattrib = (o->struct_data.npc_data.ani.display | PAL_FX_SMOKE << 9 | 3 << 12);

    // Only draw every other frame for both visibility and performance
    if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
    {
        SpriteEngine_AddToFrontLayer(o, temp_tileattrib);
    }   

    return;
}

void routines_fx_impact(struct game_object * o)
{
    SpriteEngine_AddToFrontLayer(o, (AniSystem_GetFixedFrame_Fast(o) | PAL_SYS_IMPACT << 9 | 3 << 12));

    if (system_game_paused)
    {
        return;
    }

    // Check if the object is to be destroyed
    if (o->struct_data.npc_data.ttl == 0)
    {
        obj_destroy(o->array_index);
    }
    else
    {
        // Decrement time to live
        o->struct_data.npc_data.ttl--;
    }

    return;
}

void routines_interactable_switch(struct game_object * o)
{
    if (!system_game_paused)
    {
        // Check if a player hit is on the switch

        // Only test when the switch can react (i.e. after timeout)
        // And while not in combat
        if (o->struct_data.interactable_data.delay_time == 0)
        {
            if (CollisionCheck_InteractableTestPlayerAction(o) != 0)
            {
                if (!event_in_combat_shadow)
                {
                    SoundInterface_PlaySfx(SFX_INTERACT_SWITCH,0);
                    
                    o->state ^= STATE_SWITCH_ON;
                    event_flags_local[o->struct_data.interactable_data.event_flag] = o->state;
                    o->struct_data.interactable_data.delay_time = 15 / V_MUL; // responsive enough for punch clicks

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

    SpriteEngine_AddToBackLayer(o, (0x20 + (o->state << 1)) | PAL_INTERACTABLE_SWITCH_WALL << 9 | 2 << 12);

    return;
}

void routines_interactable_sign(struct game_object * o)
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

                    UserInterface_PrintText_MultiLine(o->data_ptr, UI_MSGBOX_ML_START, UI_MARGIN_LEFT);

                    system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_MSGBOX);
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

    SpriteEngine_AddToBackLayer(o, 0x28 | PAL_INTERACTABLE_SIGN_WALL << 9 | 2 << 12);

    return;
}

void routines_interactable_treasurechest(struct game_object * o)
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
                obj_destroy(o->array_index);
            }
            else
            {
                // Decrement time to live
                o->struct_data.interactable_data.ttl--;
            }
        }
    }

    if (!o->struct_data.interactable_data.opened)
    {
        SpriteEngine_AddToSortedLayer(o, 0x24 | PAL_INTERACTABLE_TREASURECHEST << 9 | 2 << 12);
    }
    else
    {
        SpriteEngine_AddToSortedLayer(o, 0x26 | PAL_INTERACTABLE_TREASURECHEST << 9 | 2 << 12);
    }

    return;
}

void routines_interactable_blocker(struct game_object * o)
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

    if (o->state == STATE_SWITCH_OFF) // Only draw if the switch is off.
    {
        switch (o->id)
        {
            case OBJID_INTERACTABLE_BLOCKER_FLOOR:
                SpriteEngine_AddToBackLayer(o, 0x0e | PAL_INTERACTABLE_BLOCKER_FLOOR << 9 | 2 << 12);
                break;
            case OBJID_INTERACTABLE_BLOCKER_DOOR_NS:
                SpriteEngine_AddMetaSprite(o, &data_metaspr_door_ns[0]);
                break;
            case OBJID_INTERACTABLE_BLOCKER_DOOR_EW:
                SpriteEngine_AddMetaSprite(o, &data_metaspr_door_ew[0]);
                break;
        }
    }

    return;
}

void routines_spawner(struct game_object * o)
{
    if (!system_game_paused)
    {
        int16_t x1 = obj_player_pointer->pos.x.lh.h;
        int16_t y1 = obj_player_pointer->pos.y.lh.h;

        // Check if the player is within the designated box
        if (CollisionCheck_Aabb_Direct_Rectangle(x1, o->struct_data.interactable_data.spawn_area_x, y1, o->struct_data.interactable_data.spawn_area_y, 16, o->struct_data.interactable_data.spawn_area_w, 16, o->struct_data.interactable_data.spawn_area_h) == 0)
        {
            obj_instantiate_npcs((struct obj_list_entry_spawns *)o->data_ptr, o->pos.x.lh.h, o->pos.y.lh.h);

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

            bg_scroll_use_interpolation = 1;
            bg_scroll_suppress_interpolation_state_change = 2;

            obj_destroy(o->array_index);
        }
    }

    return;
}

void routines_drop_money(struct game_object * o)
{
    if (!system_game_paused)
    {
        if (AniSystem_AnimateDropGravity(o))
        {
            SoundInterface_PlaySfx(SFX_DROP_COIN,0);
        }

        if (o->pos.z.a == 0) // If item is on floor
        {
            struct game_object * p = obj_player_pointer;

            // Check if the player is within the designated box
            if (CollisionCheck_Aabb_BetweenObjects(p, o) == 0)
            {
                SoundInterface_PlaySfx(SFX_DROP_COIN,0);

                p->struct_data.npc_data.money += o->struct_data.npc_data.money;

                obj_destroy(o->array_index);
            }
        }
    }

    uint16_t temp_tileattrib;
    temp_tileattrib = (0x2a | PAL_DROP_MONEY << 9 | 2 << 12);

    SpriteEngine_AddToSortedLayer(o, temp_tileattrib);

    return;
}

void routines_drop_rec_meat(struct game_object * o)
{
    if (!system_game_paused)
    {
        if (AniSystem_AnimateDropGravity(o))
        {
            SoundInterface_PlaySfx(SFX_DROP_BOUNCE,0);
        }

        if (o->pos.z.a == 0) // If item is on floor
        {
            struct game_object * p = obj_player_pointer;
            
            // Check if the player is within the designated box
            if (CollisionCheck_Aabb_BetweenObjects(p, o) == 0)
            {
                SoundInterface_PlaySfx(SFX_DROP_BOUNCE,0);
                
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
                
                obj_destroy(o->array_index);
            }
        }
    }

    uint16_t temp_tileattrib;
    temp_tileattrib = (0x0c | PAL_DROP_REC_MEAT << 9 | 2 << 12);

    SpriteEngine_AddToSortedLayer(o, temp_tileattrib);

    return;
}

void routines_dummy(struct game_object * o)
{
    return;
}
