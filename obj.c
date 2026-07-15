#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "routines.h"
#include "routines_player.h"
#include "routines_enemy.h"
#include "routines_boss.h"
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


ZP struct game_object * obj_player_pointer;

ZP uint16_t obj_first_available;
struct game_object obj_general[OBJ_GENERAL_MAX_COUNT];

uint16_t obj_delete_queue[OBJ_GENERAL_MAX_COUNT];
uint16_t obj_delete_queue_count;

int16_t obj_player_index;
uint16_t obj_active_count;

uint16_t obj_next_uid;

// Enemy data that shouldn't be in the object area
uint16_t obj_enemies_defeated;
uint16_t obj_enemies_target_count;
uint16_t obj_enemies_max_count;

// Player only data that shouldn't be in the object area
uint16_t obj_player_attack_interval;
uint16_t obj_player_prev_facing;
uint8_t * obj_player_prev_sprframe;
uint16_t obj_player_active_fireballs;

uint16_t obj_player_health_regen_delay;
uint16_t obj_player_health_regen_interval;
uint16_t obj_player_health_regen_value;
uint16_t obj_player_health_regen_limit;

uint16_t obj_player_recovery_drop_pity;

uint16_t obj_player_upgrades_bought_hp;
uint16_t obj_player_upgrades_bought_attack;
uint16_t obj_player_upgrades_bought_defense;

uint32_t obj_player_upgrades_cost_hp;
uint32_t obj_player_upgrades_cost_attack;
uint32_t obj_player_upgrades_cost_defense;

// Hitbox data
ZP uint16_t obj_hitbox_player_first_available;
struct game_object obj_hitbox_player[OBJ_PLAYERHITBOX_MAX_COUNT];
uint16_t obj_hitbox_player_delete_queue[OBJ_PLAYERHITBOX_MAX_COUNT];
uint16_t obj_hitbox_player_delete_queue_count;
uint16_t obj_hitbox_count_player;

ZP uint16_t obj_hitbox_enemy_first_available;
struct game_object obj_hitbox_enemy[OBJ_ENEMYHITBOX_MAX_COUNT];
uint16_t obj_hitbox_enemy_delete_queue[OBJ_ENEMYHITBOX_MAX_COUNT];
uint16_t obj_hitbox_enemy_delete_queue_count;
uint16_t obj_hitbox_count_enemy;

void ObjectSystem_ProcessObjects() 
{
    event_in_combat = 0;

    snd_flame_active = 0;

    if (!system_game_paused)
    {
        if (snd_firecrackle_timeout != 0)
        {
            snd_firecrackle_timeout--;
        }
    }
    else
    {
        // Silence the looping fire sound
        if (snd_flame_playing == 1)
        {
            SoundInterface_StopSfx(SFX_ATK_FIRE_BREATH);
            snd_flame_playing = 0;
        }
    }

    // New implementation
    struct game_object * ptr = (struct game_object *)&obj_general[0];

    for (int i = 0; i < OBJ_GENERAL_MAX_COUNT; i++)
    {
        if (ptr->id == OBJID_NULL)
        {
            ptr++;
            continue;
        }

        void (*func)(struct game_object *) = ptr->func_ptr; 
        func(ptr);

        ptr++;
    }

    // Repeat for player hitboxes
    obj_player_active_fireballs = 0;

    ptr = (struct game_object *)&obj_hitbox_player[0];

    if(obj_hitbox_count_player != 0)
    {
        for (int i = 0; i < OBJ_PLAYERHITBOX_MAX_COUNT; i++)
        {
            void (*func)(struct game_object *) = ptr->func_ptr; 
            func(ptr);
            ptr++;
        }
    }

    if (obj_player_active_fireballs > 0)
    {
        Gfx_SetColorMath(obj_player_active_fireballs,0,0, true);
        gfx_cmath_change = -64 * V_MUL;
        shadow_cgwsub = 0x00;
        shadow_cgadsub = 0x32;
    }

    // Finally repeat for enemies
    ptr = (struct game_object *)&obj_hitbox_enemy[0];

    if (obj_hitbox_count_enemy != 0)
    {
        for (int i = 0; i < OBJ_ENEMYHITBOX_MAX_COUNT; i++)
        {
            void (*func)(struct game_object *) = ptr->func_ptr; 
            func(ptr);

            ptr++;
        }
    }

    event_in_combat_shadow = event_in_combat;

    if ((event_in_combat == 0) && 
    (!system_game_paused) &&
    (bg_scroll_x_bounds_min.full.high.a != -32768) &&
    (bg_scroll_y_bounds_min.full.high.a != -32768))
    {
        if (bg_scroll_suppress_interpolation_state_change)
        {
            bg_scroll_suppress_interpolation_state_change--;
        }

        if (!bg_scroll_suppress_interpolation_state_change)
        {
            bg_scroll_x_bounds_min.full.high.a = -32768;
            bg_scroll_y_bounds_min.full.high.a = -32768;
            bg_scroll_use_interpolation = true;
        }
    }

    if (snd_flame_active == 0)
    {
        if (snd_flame_playing == 1)
        {
            SoundInterface_StopSfx(SFX_ATK_FIRE_BREATH);
            snd_flame_playing = 0;
        }
    }

    return;
}

/*
    Reset object memory thoroughly (completely wipe)
*/
void ObjectSystem_ResetStandardObjects(int start_index)
{
    REG_DMAP7 = 0x08; // byte reg write, fixed
    REG_BBAD7 = 0x80; // WMDATA

    REG_A1B7 = (uint8_t)((uint32_t)&const_zero >> 16);
    
    REG_WMADDH = (uint8_t)((uint32_t)&obj_general[start_index] >> 16);

    REG_WMADDLM = (uint16_t)((uint32_t)&obj_general[start_index]);

    REG_A1T7LH = (uint16_t)((uint32_t)&const_zero);
    REG_DAS7LH = ((OBJ_GENERAL_MAX_COUNT - start_index) * (uint16_t)sizeof(struct game_object));

    System_Hsync(0);
    
    REG_MDMAEN = 0x80;

    // Then initialize the next pointers and function pointers for all of them
    for (int i = start_index; i < (OBJ_GENERAL_MAX_COUNT - 1); i++)
    {
        obj_general[i].next_free = i+1;
        obj_general[i].func_ptr = (void *)&Routines_Dummy;
    }

    obj_general[OBJ_GENERAL_MAX_COUNT - 1].next_free = 0xffff;
    obj_general[OBJ_GENERAL_MAX_COUNT - 1].func_ptr = (void *)&Routines_Dummy;

    obj_first_available = start_index;

    return;
}

/*
    Player hitboxes (e.g. fireballs) reset too, in a separate list for performance reasons
*/
void ObjectSystem_ResetPlayerHitboxes()
{
    REG_DMAP7 = 0x08; // byte reg write, fixed
    REG_BBAD7 = 0x80; // WMDATA

    REG_A1B7 = (uint8_t)((uint32_t)&const_zero >> 16);
    
    REG_WMADDH = (uint8_t)((uint32_t)&obj_hitbox_player[0] >> 16);

    REG_WMADDLM = (uint16_t)((uint32_t)&obj_hitbox_player[0]);

    REG_A1T7LH = (uint16_t)((uint32_t)&const_zero);
    REG_DAS7LH = (OBJ_PLAYERHITBOX_MAX_COUNT * (uint16_t)sizeof(struct game_object));

    System_Hsync(0);
    
    REG_MDMAEN = 0x80;

    // Then initialize the next pointers and function pointers for all of them
    for (int i = 0; i < (OBJ_PLAYERHITBOX_MAX_COUNT - 1); i++)
    {
        obj_hitbox_player[i].next_free = i+1;
        obj_hitbox_player[i].func_ptr = (void *)&Routines_Dummy;
    }

    obj_hitbox_player[OBJ_PLAYERHITBOX_MAX_COUNT - 1].next_free = 0xffff;
    obj_hitbox_player[OBJ_PLAYERHITBOX_MAX_COUNT - 1].func_ptr = (void *)&Routines_Dummy;

    obj_hitbox_player_first_available = 0;

    return;
}

/*
    Do the same for enemy hitboxes too
*/
void ObjectSystem_ResetEnemyHitboxes()
{
    REG_DMAP7 = 0x08; // byte reg write, fixed
    REG_BBAD7 = 0x80; // WMDATA

    REG_A1B7 = (uint8_t)((uint32_t)&const_zero >> 16);
    
    REG_WMADDH = (uint8_t)((uint32_t)&obj_hitbox_enemy[0] >> 16);

    REG_WMADDLM = (uint16_t)((uint32_t)&obj_hitbox_enemy[0]);

    REG_A1T7LH = (uint16_t)((uint32_t)&const_zero);
    REG_DAS7LH = (OBJ_ENEMYHITBOX_MAX_COUNT * (uint16_t)sizeof(struct game_object));

    System_Hsync(0);
    
    REG_MDMAEN = 0x80;

    // Then initialize the next pointers and function pointers for all of them
    for (int i = 0; i < (OBJ_ENEMYHITBOX_MAX_COUNT - 1); i++)
    {
        obj_hitbox_enemy[i].next_free = i+1;
        obj_hitbox_enemy[i].func_ptr = (void *)&Routines_Dummy;
    }

    obj_hitbox_enemy[OBJ_ENEMYHITBOX_MAX_COUNT - 1].next_free = 0xffff;
    obj_hitbox_enemy[OBJ_ENEMYHITBOX_MAX_COUNT - 1].func_ptr = (void *)&Routines_Dummy;

    obj_hitbox_enemy_first_available = 0;

    return;
}

/*! \def ObjectSystem_InstantiateObject

    \brief Creates a new object with the object ID passed at pixel level location X and Y.
*/
int16_t ObjectSystem_InstantiateObject(
    uint16_t id,
    int16_t x,
    int16_t y,
    uint16_t local_event_flag
)
{
    uint16_t i = obj_first_available;
    if (obj_first_available == 0xffff)
    {
        return -1;
    }
    obj_first_available = obj_general[i].next_free;

    uint16_t j = 0;
    // perform additional checks for sprite slots

    if ((id != OBJID_PLAYER) && (id != OBJID_BOSS_TEST1))
    {
        if (((id >= OBJID_START_OF_DMA_SPRITES) && (id <= OBJID_END_OF_DMA_SPRITES)) ||
            ((id >= OBJID_START_OF_DMA_LIGHT_SPRITES) && (id <= OBJID_END_OF_DMA_LIGHT_SPRITES)) )
        {
            uint16_t k = SpriteEngine_GetVramSlot16(i);
            
            if (k >= 128)
            {
                // Out of VRAM slots
                return -1;
            }

            //if relevant bits are taken to as they shift all the way to bit 0 first for the lowest bit:
            uint16_t temp_tilenum = 0; 
            temp_tilenum |= (k & 0xf0) << 2;
            temp_tilenum |= (k & 0x0c);
            temp_tilenum |= (k & 0x02) << 4;
            temp_tilenum |= (k & 0x01) << 1;
            // (RRRR * 64) + (CC * 4) + (r * 32) + (c * 2);

            obj_general[i].struct_data.npc_data.tilenum = temp_tilenum;
            obj_general[i].struct_data.npc_data.vram_addr = temp_tilenum << 4; // mul by 16, size of a 8x8 tile - this is in words
        }
    }

    struct game_object * p = &obj_general[i];

    p->id = id;
    p->uid = ObjectSystem_GetUid();
    p->array_index = i;
    
    p->pos.x.a = ((int32_t)x) << 16;
    p->pos.y.a = ((int32_t)y) << 16;

    p->struct_data.npc_data.ttl = 0; // always reset

    p->struct_data.npc_data.ani.frame = 0;

    if ((id >= OBJID_CONST_END_OF_UNSORTED_SPRITES) && ((id != OBJID_HITBOX_INVISIBLE) || (id != OBJID_HITBOX_INVISIBLE_E))) // mini objects don't need these
    {
        p->delta.x.a = 0;
        p->delta.y.a = 0;

        p->pos.z.a = 0;
        p->delta.z.a = 0;
        
        p->state = STATE_IDLE;

        p->struct_data.npc_data.status = STATUS_NORMAL;
        p->struct_data.npc_data.status_time = 0;
        p->struct_data.npc_data.invuln_time = 0;

        p->struct_data.npc_data.ai_state = AI_STATE_IDLE;
        p->struct_data.npc_data.ai_timer = 0;

        p->facing = FACING_DOWN;

        p->struct_data.npc_data.ani.display = (uint16_t)((uint32_t)AniSystem_GetDynamicFrame(p));
    }
    else
    {
        p->struct_data.npc_data.ani.display = 0x5d; // empty tile in fixed mode
    }

    if ((id >= OBJID_START_OF_INTERACTABLES) && (id <= OBJID_END_OF_INTERACTABLES))
    {
        p->state = STATE_SWITCH_OFF;
        p->tile.x = (uint16_t)p->pos.x.lh.h >> 4;
        p->tile.y = (uint16_t)p->pos.y.lh.h >> 4;

        if (id != OBJID_INTERACTABLE_LEVEL_WARP)
        {
            p->struct_data.interactable_data.event_flag = local_event_flag;
        }
        
        p->struct_data.interactable_data.money = 0;
        p->struct_data.interactable_data.opened = false;
    }

    if (id == OBJID_PLAYER)
    {
        // Keep track of the previous image DMA'd
        p->struct_data.npc_data.ani.last_address = 0;
        p->struct_data.npc_data.ani.last_dmafailed = 0;

        p->struct_data.npc_data.money = 0;

        p->struct_data.npc_data.hp = PLAYER_HEALTH_STARTING;
        p->struct_data.npc_data.hp_max = PLAYER_HEALTH_STARTING;

        p->struct_data.npc_data.attack = PLAYER_ATTACK_VALUE;
        p->struct_data.npc_data.defense = PLAYER_DEFENSE_VALUE;

        p->w = 16;
        p->h = 16;
    }

    // Send the ID to a helper function to set the correct data.
    // If the function returns false, this is not an 
    // enemy and set things
    if (ObjectSystem_GetEnemyData(p))
    {
        p->struct_data.npc_data.ani.last_address = 0;
        p->struct_data.npc_data.ani.last_dmafailed = 0;
        p->struct_data.npc_data.hp_tile_offset = 0;
        p->struct_data.npc_data.hp_display_time = 0;

        p->state = STATE_SPAWNING;
        p->struct_data.npc_data.status_time = 64 / V_MUL;

        if (id == OBJID_BOSS_TEST1)
        {
            SpriteEngine_GetVramForBoss();
            
            Routines_Boss_Init();
        }
    }
    else
    {
        if ((id == OBJID_DROP_MONEY) || (id == OBJID_DROP_REC_MEAT))
        {
            p->w = 16;
            p->h = 16;
        }

        if ((id == OBJID_FIREBALL) || (id == OBJID_HITBOX_INVISIBLE))
        {
            p->hit_type = 0x0001;

            p->w = 16;
            p->h = 16;
        }
        else if ((id == OBJID_HITBOX_INVISIBLE_E) || (id == OBJID_BUBBLE_E) || (id == OBJID_ARROW_E))
        {
            p->hit_type = 0x8001;

            p->w = 16;
            p->h = 16;
        }
        else
        {
            p->hit_type = 0x0000;
        }

        if ((id == OBJID_INTERACTABLE_BLOCKER_FLOOR) || 
            (id == OBJID_INTERACTABLE_BLOCKER_DOOR_EW) || 
            (id == OBJID_INTERACTABLE_BLOCKER_DOOR_NS))
        {
            // Set a non-zero value
            p->struct_data.interactable_data.delay_time = 0xffff;
        }
    }

    obj_active_count++;

    ObjectSystem_SetFunctionPointer(p);

    p->r = p->pos.x.lh.h + p->w;
    p->b = p->pos.y.lh.h + p->h;

    return i;
}

/*
    Player hitboxes should call this instead
*/
int16_t ObjectSystem_InstantiatePlayerHitbox(
    uint16_t id,
    int16_t x,
    int16_t y
)
{
    uint16_t i = obj_hitbox_player_first_available;
    if (obj_hitbox_player_first_available == 0xffff)
    {
        return -1;
    }
    obj_hitbox_player_first_available = obj_hitbox_player[i].next_free;

    uint16_t j = 0;

    // Player hitboxes don't need VRAM
    struct game_object * p = &obj_hitbox_player[i];

    p->id = id;
    p->uid = ObjectSystem_GetUid(); 
    p->array_index = i;
    
    p->pos.x.a = ((int32_t)x) << 16;
    p->pos.y.a = ((int32_t)y) << 16;

    p->struct_data.npc_data.ttl = 0; // always reset
    
    p->struct_data.npc_data.ani.frame = 0;

    p->pos.z.a = 0;
    p->delta.z.a = 0;

    p->struct_data.npc_data.ani.display = 0x5d; // empty tile in fixed mode
    p->struct_data.npc_data.ani.last_address = 0; // make this invalid

    //obj_active_count++;
    obj_hitbox_count_player++;

    p->hit_type = 0x0001;

    p->w = 16;
    p->h = 16;

    ObjectSystem_SetFunctionPointer(p);

    p->r = p->pos.x.lh.h + p->w;
    p->b = p->pos.y.lh.h + p->h;

    return i;
}

/*
    And use this for enemy hitboxes
*/
int16_t ObjectSystem_InstantiateEnemyHitbox(
    uint16_t id,
    int16_t x,
    int16_t y
)
{
    uint16_t i = obj_hitbox_enemy_first_available;
    if (obj_hitbox_enemy_first_available == 0xffff)
    {
        return -1;
    }

    // perform additional checks for sprite slots
    if ((id >= OBJID_START_OF_DMA_LIGHT_SPRITES) && (id <= OBJID_END_OF_DMA_LIGHT_SPRITES))
    {
        uint16_t k = SpriteEngine_GetVramSlot16(OBJ_GENERAL_MAX_COUNT + i);
        
        if (k >= 128)
        {
            // Out of VRAM slots
            return -1;
        }

        obj_hitbox_enemy_first_available = obj_hitbox_enemy[i].next_free;

        //if relevant bits are taken to as they shift all the way to bit 0 first for the lowest bit:
        uint16_t temp_tilenum = 0; 
        temp_tilenum |= (k & 0xf0) << 2;
        temp_tilenum |= (k & 0x0c);
        temp_tilenum |= (k & 0x02) << 4;
        temp_tilenum |= (k & 0x01) << 1;
        // (RRRR * 64) + (CC * 4) + (r * 32) + (c * 2);

        obj_hitbox_enemy[i].struct_data.npc_data.tilenum = temp_tilenum;
        obj_hitbox_enemy[i].struct_data.npc_data.vram_addr = temp_tilenum << 4; // mul by 16, size of a 8x8 tile - this is in words
    }
    else
    {
        obj_hitbox_enemy_first_available = obj_hitbox_enemy[i].next_free;
    }

    struct game_object * p = &obj_hitbox_enemy[i];

    p->id = id;
    p->uid = ObjectSystem_GetUid(); 
    p->array_index = i;
    
    p->pos.x.a = ((int32_t)x) << 16;
    p->pos.y.a = ((int32_t)y) << 16;

    p->struct_data.npc_data.ttl = 0; // always reset
    
    p->struct_data.npc_data.ani.frame = 0;

    p->pos.z.a = 0;
    p->delta.z.a = 0;

    p->state = STATE_IDLE;
    p->facing = FACING_DOWN;
    p->struct_data.npc_data.ani.display = (uint16_t)((uint32_t)AniSystem_GetDynamicFrame_Stateless(p));
    p->struct_data.npc_data.ani.last_address = 0; // make this invalid

    obj_hitbox_count_enemy++;

    p->hit_type = 0x8001;

    p->w = 16;
    p->h = 16;

    ObjectSystem_SetFunctionPointer(p);

    p->r = p->pos.x.lh.h + p->w;
    p->b = p->pos.y.lh.h + p->h;

    return i;
}

uint16_t ObjectSystem_List_InstantiateNpcs(const struct obj_list_entry_spawns* list, int16_t offset_x, int16_t offset_y)
{
    while (list->id != OBJID_NULL)
    {
        if (obj_active_count >= OBJ_GENERAL_MAX_COUNT)
        {
            return 1;
        }

        uint16_t temp_objid = list->id;
        int16_t temp_x;
        int16_t temp_y;

        if (list->random_spread != 0)
        {
            int16_t temp_rand = (((int16_t)Math_GetRandom_u16()) % list->random_spread) - (list->random_spread >> 1);

            temp_x = list->x + temp_rand + offset_x;
            temp_y = list->y + temp_rand + offset_y;
        }
        else
        {
            temp_x = list->x + offset_x;
            temp_y = list->y + offset_y;
        }

        if (ObjectSystem_InstantiateObject(temp_objid, temp_x, temp_y, 0) == -1)
        {
            return 1;
        }

        list++;
    }

    return 0;
}

uint16_t ObjectSystem_List_InstantiateSpawners(const struct obj_list_entry_spawners* list)
{
    uint16_t temp_total_spawns = 0;
    
    while (list->id != OBJID_NULL)
    {
        if (obj_active_count >= OBJ_GENERAL_MAX_COUNT)
        {
            break;
        }

        uint16_t temp_objid = list->id;
        int16_t temp_x = list->x;
        int16_t temp_y = list->y;

        int16_t index = ObjectSystem_InstantiateObject(temp_objid, temp_x, temp_y, 0);

        if (index == -1)
        {
            break;
        }

        obj_general[index].struct_data.interactable_data.spawn_area_x = list->x;
        obj_general[index].struct_data.interactable_data.spawn_area_y = list->y;
        obj_general[index].struct_data.interactable_data.spawn_area_w = list->w;
        obj_general[index].struct_data.interactable_data.spawn_area_h = list->h;
        obj_general[index].struct_data.interactable_data.screen_x = list->screen_x;
        obj_general[index].struct_data.interactable_data.screen_y = list->screen_y;
        obj_general[index].struct_data.interactable_data.screen_w = list->screen_w;
        obj_general[index].struct_data.interactable_data.screen_h = list->screen_h;
        
        obj_general[index].data_ptr = (uint8_t *)list->spawn_list;

        struct obj_list_entry_spawns * temp_ptr = (struct obj_list_entry_spawns *)list->spawn_list;

        while (temp_ptr->id != OBJID_NULL)
        {
            temp_total_spawns++;
            temp_ptr++;
        }

        list++;
    }

    if (temp_total_spawns > 0)
    {
        obj_enemies_max_count = temp_total_spawns;

        // Debug: make this much smaller
        obj_enemies_target_count = (uint16_t)((float)(obj_enemies_max_count) * 0.3f);

        //obj_enemies_target_count = (uint16_t)((float)(obj_enemies_max_count) * 0.6f);

        if (obj_enemies_target_count == 0)
        {
            obj_enemies_target_count++;
        }
    }
    else
    {
        obj_enemies_max_count = 0;
        obj_enemies_target_count = 0;
    }

    return 0;
}

uint16_t ObjectSystem_List_InstantiateInteractables(const struct obj_list_entry_interactable* list)
{
    while (list->id != OBJID_NULL)
    {
        uint16_t temp_objid = list->id;
        int16_t temp_x = list->x;
        int16_t temp_y = list->y;
        uint16_t temp_flag = (uint16_t)((uint32_t)(list->flag));

        if (obj_active_count >= OBJ_GENERAL_MAX_COUNT)
        {
            return 1;
        }

        uint16_t index = ObjectSystem_InstantiateObject(temp_objid, temp_x, temp_y, temp_flag);
        if (index == -1)
        {
            return 1;
        }

        if (temp_objid == OBJID_INTERACTABLE_SIGN_WALL)
        {
            uint8_t * string_ptr = (uint8_t *)(list->flag);
            obj_general[index].data_ptr = string_ptr;
        }
        else if (temp_objid == OBJID_INTERACTABLE_TREASURECHEST)
        {
            obj_general[index].struct_data.interactable_data.money = (uint32_t)list->flag;
        }
        else if (temp_objid == OBJID_INTERACTABLE_LEVEL_WARP)
        {
            uint8_t * level_ptr = (uint8_t *)(list->flag);
            obj_general[index].data_ptr = level_ptr;
        }

        list++;
    }

    return 0;
}

/*! \def ObjectSystem_DestroyStandardObject

    \brief Adds the object whose slot i into the deletion queue.
*/
void ObjectSystem_DestroyStandardObject(uint16_t i)
{
    obj_delete_queue[obj_delete_queue_count] = i;

    obj_delete_queue_count++;

    return;
}

// Ditto for player hitboxes
void ObjectSystem_DestroyPlayerHitbox(uint16_t i)
{
    obj_hitbox_player_delete_queue[obj_hitbox_player_delete_queue_count] = i;

    obj_hitbox_player_delete_queue_count++;

    return;
}

// Lastly for enemy hitboxes
void ObjectSystem_DestroyEnemyHitbox(uint16_t i)
{
    obj_hitbox_enemy_delete_queue[obj_hitbox_enemy_delete_queue_count] = i;

    obj_hitbox_enemy_delete_queue_count++;

    return;
}

/*! \def ObjectSystem_CleanupStandardObjects

    \brief Deletes all objects whose slot i is within the deletion queue.
*/
void ObjectSystem_CleanupStandardObjects()
{
    for (int i = 0; i < obj_delete_queue_count; i++)
    {
        // TODO: use a function call return value
        if ((obj_general[obj_delete_queue[i]].id == OBJID_SLIME) || (obj_general[obj_delete_queue[i]].id == OBJID_LIZARDMAN))
        {
            SpriteEngine_ReleaseVramSlot(obj_delete_queue[i], 1);
        }

        obj_general[obj_delete_queue[i]].id = OBJID_NULL;

        // Thread back the object to the next free
        obj_general[obj_delete_queue[i]].next_free = obj_first_available;
        obj_first_available = obj_delete_queue[i];

        // Fix the object function to dummy
        obj_general[obj_delete_queue[i]].func_ptr = (void *)&Routines_Dummy;
    }
    
    obj_active_count -= obj_delete_queue_count;

    obj_delete_queue_count = 0;
    return;
}

void ObjectSystem_CleanupPlayerHitboxes()
{
    for (int i = 0; i < obj_hitbox_player_delete_queue_count; i++)
    {
        obj_hitbox_player[obj_hitbox_player_delete_queue[i]].id = OBJID_NULL;

        // Thread back the object to the next free
        obj_hitbox_player[obj_hitbox_player_delete_queue[i]].next_free = obj_hitbox_player_first_available;
        obj_hitbox_player_first_available = obj_hitbox_player_delete_queue[i];

        // Fix the object function to dummy
        obj_hitbox_player[obj_hitbox_player_delete_queue[i]].func_ptr = (void *)&Routines_Dummy;
    }
    
    obj_hitbox_count_player -= obj_hitbox_player_delete_queue_count;

    obj_hitbox_player_delete_queue_count = 0;
    return;
}

void ObjectSystem_CleanupEnemyHitboxes()
{
    for (int i = 0; i < obj_hitbox_enemy_delete_queue_count; i++)
    {
        unsigned int id = obj_hitbox_enemy[obj_hitbox_enemy_delete_queue[i]].id;
        if ((id >= OBJID_START_OF_DMA_LIGHT_SPRITES) && (id <= OBJID_END_OF_DMA_LIGHT_SPRITES))
        {
            SpriteEngine_ReleaseVramSlot(OBJ_GENERAL_MAX_COUNT + obj_hitbox_enemy_delete_queue[i], 1);
        }

        obj_hitbox_enemy[obj_hitbox_enemy_delete_queue[i]].id = OBJID_NULL;

        // Thread back the object to the next free
        obj_hitbox_enemy[obj_hitbox_enemy_delete_queue[i]].next_free = obj_hitbox_enemy_first_available;
        obj_hitbox_enemy_first_available = obj_hitbox_enemy_delete_queue[i];

        // Fix the object function to dummy
        obj_hitbox_enemy[obj_hitbox_enemy_delete_queue[i]].func_ptr = (void *)&Routines_Dummy;
    }
    
    obj_hitbox_count_enemy -= obj_hitbox_enemy_delete_queue_count;

    obj_hitbox_enemy_delete_queue_count = 0;
    return;
}

uint16_t ObjectSystem_GetUid()
{
    uint16_t temp_uid = obj_next_uid;
    obj_next_uid++;

    if (obj_next_uid == 0)
    {
        obj_next_uid++;
    }

    return temp_uid;
}

void ObjectSystem_SetFunctionPointer(struct game_object * o)
{
    switch (o->id)
    {
        case OBJID_NULL:
            o->func_ptr = (void *)&Routines_Dummy;
            break;
        case OBJID_PLAYER:
            o->func_ptr = (void *)&Routines_Player;
            break;
        case OBJID_FIREBALL:
            o->func_ptr = (void *)&Routines_Player_Fireball;
            break;
        case OBJID_SLIME:
            o->func_ptr = (void *)&Routines_Enemy_Slime;
            break;
        case OBJID_BUBBLE_E:
            o->func_ptr = (void *)&Routines_Enemy_Slime_Bubble;
            break;
        case OBJID_ARROW_E:
            o->func_ptr = (void *)&Routines_Enemy_Lizardman_Arrow;
            break;
        case OBJID_LIZARDMAN:
            o->func_ptr = (void *)&Routines_Enemy_Lizardman;
            break;
        case OBJID_BOSS_TEST1:
            o->func_ptr = (void *)&Routines_Boss_Test;
            break;
        case OBJID_BOSS_TEST1_ATTACK1:
            o->func_ptr = (void *)&Routines_Boss_Test_Attack_Particle;
            break;
        case OBJID_FX_SMOKE:
            o->func_ptr = (void *)&Routines_Fx_Smoke;
            break;
        case OBJID_DROP_REC_MEAT:
            o->func_ptr = (void *)&Routines_Drops_Recovery_Meat;
            break;
        case OBJID_DROP_MONEY:
            o->func_ptr = (void *)&Routines_Drops_Money;
            break;
        case OBJID_HITBOX_INVISIBLE:
            o->func_ptr = (void *)&Routines_Player_InvisibleHit;
            break;
        case OBJID_HITBOX_INVISIBLE_E:
            o->func_ptr = (void *)&Routines_Enemy_InvisibleHit;
            break;
        case OBJID_SYS_IMPACT:
            o->func_ptr = (void *)&Routines_Fx_Impact;
            break;
        case OBJID_INTERACTABLE_SWITCH_WALL:
        case OBJID_INTERACTABLE_SWITCH_FLOOR:
            o->func_ptr = (void *)&Routines_Interactables_Switch;
            break;
        case OBJID_INTERACTABLE_BLOCKER_FLOOR:
        case OBJID_INTERACTABLE_BLOCKER_DOOR_NS:
        case OBJID_INTERACTABLE_BLOCKER_DOOR_EW:
            o->func_ptr = (void *)&Routines_Interactable_Blocker;
            break;
        case OBJID_INTERACTABLE_LEVEL_WARP:
            o->func_ptr = (void *)&Routines_LevelWarp;
            break;
        case OBJID_INTERACTABLE_SIGN_WALL:
            o->func_ptr = (void *)&Routines_Interactable_Sign;
            break;
        case OBJID_SPAWNER_ENEMY:
            o->func_ptr = (void *)&Routines_EnemySpawner;
            break;
        case OBJID_INTERACTABLE_TREASURECHEST:
            o->func_ptr = (void *)&Routines_TreasureChest;
            break;
        default:
            // Unimplemented object
            o->func_ptr = (void *)&Routines_Dummy;
            break;
    }

    return;
}

bool ObjectSystem_GetEnemyData(struct game_object * o)
{
    struct enemy_data * data_ptr;

    switch (o->id)
    {
        case OBJID_SLIME:
            data_ptr = (struct enemy_data *)&data_enemy_stats_slime;
            break;
        case OBJID_LIZARDMAN:
            data_ptr = (struct enemy_data *)&data_enemy_stats_lizardman;
            break;
        case OBJID_LIZARDMAN_ARCHER:
            data_ptr = (struct enemy_data *)&data_enemy_stats_lizardman_archer;
            break;
        case OBJID_LIZARDMAN_LILSIS:
            data_ptr = (struct enemy_data *)&data_enemy_stats_lizardman_lilsis;
            break;
        case OBJID_BOSS_TEST1:
            data_ptr = (struct enemy_data *)&data_enemy_stats_boss_0;
            break;
        default:
            return false;
    }

    o->struct_data.npc_data.hp = data_ptr->hp;
    o->struct_data.npc_data.hp_max = data_ptr->hp;

    o->struct_data.npc_data.attack = data_ptr->attack;
    o->struct_data.npc_data.defense = data_ptr->defense;

    o->struct_data.npc_data.hp_cache = data_ptr->hp;

    uint8_t temp_weight = (uint8_t)Math_GetRandom_u16();
    o->struct_data.npc_data.money = (data_ptr->money_min + (((data_ptr->money_max - data_ptr->money_min) * temp_weight) / 255));

    o->w = data_ptr->width;
    o->h = data_ptr->height;

    return true;
}