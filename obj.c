#include <stdint.h>

#include "vars.h"

#include "routines.h"
#include "obj.h"
#include "hittest.h"

#include "ani.h"

#include "map.h"
#include "spr.h"

#include "math_int.h"

#include "snd.h"
#include "consts_snd.h"
#include "system.h"

void obj_run() 
{
    hitbox_count_enemy = 0;

    obj_process_count = 0;
    hitbox_process_count_player = 0;

    blocker_build_count = 0;
    
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
            snd_stop_sfx(SFX_ATK_FIRE_BREATH);
            snd_flame_playing = 0;
        }
    }

    // New implementation
    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
	    "\tx16\n"
        "\tphy\n"
        "\tldx #<_objects\n"
        "\tldy #0\n"
        "\tcpy _obj_active_count\n"
        "\tbcs .object_break_loop\n"
        ".object_process_loop:\n"
        "\tlda $7e0000,x\n" // assumes object memory is in bank 7e
        "\tbeq .object_process_increment\n"
        "\tiny\n"
        "\tlda $7e0033,x\n"
        "\tsta >_system_JMLCodeInWRAM+2\n"
        "\tlda $7e0032,x\n"
        "\tsta >_system_JMLCodeInWRAM+1\n"
        "\tphy\n"
        "\tphx\n"
        "\ttxa\n"
        "\tldx #^_objects\n"
        "\tjsl >_system_JMLCodeInWRAM\n"
        "\tplx\n"
        "\tply\n"
        ".object_process_increment:\n"
        "\ttxa\n"
        "\tclc\n"
        "\tadc #128\n"
        "\tcmp #<_objects+6144\n"
        "\tbcs .object_break_loop\n"
        "\ttax\n"
        "\tcpy _obj_active_count\n"
        "\tbcc .object_process_loop\n"
        ".object_break_loop:\n"
        "\tply\n");
    #else
        struct game_object * ptr = (struct game_object *)&objects[0];

        for (int i = 0; i < OBJ_MAX_COUNT; i++)
        {
            if (ptr->id == OBJID_NULL)
            {
                ptr++;
                continue;
            }

            void (*func)(struct game_object *) = ptr->func_ptr; 
            func(ptr);
            obj_process_count++;

            ptr++;

            if (obj_process_count >= obj_active_count)
            {
                break;
            }
        }
    #endif

    // Repeat for hitboxes
    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
	    "\tx16\n"
        "\tphy\n"
        "\tldx #<_hitbox_player\n"
        "\tldy #0\n"
        "\tcpy _hitbox_count_player\n"
        "\tbcs .hitbox_player_break_loop\n"
        ".hitbox_player_process_loop:\n"
        "\tlda $7e0000,x\n" // assumes object memory is in bank 7e
        "\tbeq .hitbox_player_process_increment\n"
        "\tiny\n"
        "\tlda $7e0033,x\n"
        "\tsta >_system_JMLCodeInWRAM+2\n"
        "\tlda $7e0032,x\n"
        "\tsta >_system_JMLCodeInWRAM+1\n"
        "\tphy\n"
        "\tphx\n"
        "\ttxa\n"
        "\tldx #^_hitbox_player\n"
        "\tjsl >_system_JMLCodeInWRAM\n"
        "\tplx\n"
        "\tply\n"
        ".hitbox_player_process_increment:\n"
        "\ttxa\n"
        "\tclc\n"
        "\tadc #128\n"
        "\tcmp #<_hitbox_player+2048\n"
        "\tbcs .hitbox_player_break_loop\n"
        "\ttax\n"
        "\tcpy _hitbox_count_player\n"
        "\tbcc .hitbox_player_process_loop\n"
        ".hitbox_player_break_loop:\n"
        "\tply\n");
    #else
        ptr = (struct game_object *)&hitbox_player[0];

        for (int i = 0; i < HIT_MAX_COUNT; i++)
        {
            if (ptr->id == OBJID_NULL)
            {
                ptr++;
                continue;
            }

            void (*func)(struct game_object *) = ptr->func_ptr; 
            func(ptr);

            hitbox_process_count_player++;

            ptr++;

            if (hitbox_process_count_player >= hitbox_count_player)
            {
                break;
            }
        }
    #endif

    blocker_build_count_shadow = blocker_build_count;
    event_in_combat_shadow = event_in_combat;

    if ((event_in_combat == 0) && 
    (system_current_routine != ROUTINE_MSGBOX) &&
    (bg_scroll_x_bounds_min.full.high.a != -32768) &&
    (bg_scroll_y_bounds_min.full.high.a != -32768))
    {
        if (bg_scroll_suppress_interpolation_state_change != 0)
        {
            bg_scroll_suppress_interpolation_state_change--;
        }

        if (bg_scroll_suppress_interpolation_state_change == 0)
        {
            bg_scroll_x_bounds_min.full.high.a = -32768;
            bg_scroll_y_bounds_min.full.high.a = -32768;
            bg_scroll_use_interpolation = 1;
        }
    }

    if (snd_flame_active == 0)
    {
        if (snd_flame_playing == 1)
        {
            snd_stop_sfx(SFX_ATK_FIRE_BREATH);
            snd_flame_playing = 0;
        }
    }

    hitbox_count_enemy_shadow = hitbox_count_enemy;
    hitbox_count_player_shadow = hitbox_count_player;

    return;
}

/*
    Reset object memory thoroughly (completely wipe)
*/
void obj_reset(int start_index)
{
    #if VBCC_ASM == 1 // Write the first byte as zero, then use MVN to copy the rest.
        __asm(
            "\ta16\n"
            "\tx16\n"
            "\txba\n" 
            "\tlsr\n" // mul 128
            "\tsta r0\n" // offset of start, also subtract length with this
            "\tlda #6144\n"
            "\tsec\n"
            "\tsbc r0\n"
            "\tsta r1\n" // actual transfer length
            "\tphy\n"
            "\tphb\n"
            "\ta8\n"
            "\tsep #$20\n"
            "\tlda #$00\n"
            "\tsta >_objects\n" // write first zero byte
            "\tlda #^_objects\n"
            "\tsta >_system_MVNCodeInWRAM+1\n" // write bank byte of source 
            "\tsta >_system_MVNCodeInWRAM+2\n" // ditto for destination
            "\ta16\n"
            "\trep #$21\n"
            "\tlda r0\n" // load source address
            "\tadc #<_objects\n" 
            "\ttax\n" 
            "\ttay\n" 
            "\tiny\n" // destination address
            "\tlda r1\n" // load transfer length
            "\tdec\n" 
            "\tdec\n" // Decrement by 2 to remove the first byte and MVN implied byte
            "\tjsl >_system_MVNCodeInWRAM;\n"
            "\tplb\n"
            "\tply\n");
    #else
        uint8_t * ptr = (uint8_t *)&objects[start_index];
        for (int i = start_index; i < (OBJ_MAX_COUNT * (uint16_t)sizeof(struct game_object)); i++)
        {
            *ptr = 0x00;
            ptr++;
        }
    #endif

    // Then initialize the next pointers and function pointers for all of them
    for (int i = start_index; i < (OBJ_MAX_COUNT - 1); i++)
    {
        objects[i].next_free = i+1;
        objects[i].func_ptr = (void *)&routines_dummy;
    }

    objects[OBJ_MAX_COUNT - 1].next_free = 0xffff;
    objects[OBJ_MAX_COUNT - 1].func_ptr = (void *)&routines_dummy;

    obj_first_available = 0;

    return;
}

/*
    Player hitboxes (e.g. fireballs) reset too, in a separate list for performance reasons
*/
void obj_reset_hit_list()
{
    #if VBCC_ASM == 1 // Write the first byte as zero, then use MVN to copy the rest.
        __asm(
            "\ta16\n"
            "\tx16\n"
            "\tstz r0\n" // offset of start, also subtract length with this
            "\tlda #2048\n"
            "\tsec\n"
            "\tsbc r0\n"
            "\tsta r1\n" // actual transfer length
            "\tphy\n"
            "\tphb\n"
            "\ta8\n"
            "\tsep #$20\n"
            "\tlda #$00\n"
            "\tsta >_hitbox_player\n" // write first zero byte
            "\tlda #^_hitbox_player\n"
            "\tsta >_system_MVNCodeInWRAM+1\n" // write bank byte of source 
            "\tsta >_system_MVNCodeInWRAM+2\n" // ditto for destination
            "\ta16\n"
            "\trep #$21\n"
            "\tlda r0\n" // load source address
            "\tadc #<_hitbox_player\n" 
            "\ttax\n" 
            "\ttay\n" 
            "\tiny\n" // destination address
            "\tlda r1\n" // load transfer length
            "\tdec\n" 
            "\tdec\n" // Decrement by 2 to remove the first byte and MVN implied byte
            "\tjsl >_system_MVNCodeInWRAM;\n"
            "\tplb\n"
            "\tply\n");
    #else
        uint8_t * ptr = (uint8_t *)&hitbox_player[0];
        for (int i = 0; i < (HIT_MAX_COUNT * (uint16_t)sizeof(struct game_object)); i++)
        {
            *ptr = 0x00;
            ptr++;
        }
    #endif

    // Then initialize the next pointers and function pointers for all of them
    for (int i = 0; i < (HIT_MAX_COUNT - 1); i++)
    {
        hitbox_player[i].next_free = i+1;
        hitbox_player[i].func_ptr = (void *)&routines_dummy;
    }

    hitbox_player[HIT_MAX_COUNT - 1].next_free = 0xffff;
    hitbox_player[HIT_MAX_COUNT - 1].func_ptr = (void *)&routines_dummy;

    hitbox_player_first_available = 0;

    return;
}

/*! \def obj_instantiate

    \brief Creates a new object with the object ID passed at pixel level location X and Y.
*/
int16_t obj_instantiate(
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
    obj_first_available = objects[i].next_free;

    uint16_t j = 0;
    // perform additional checks for sprite slots

    if (id != OBJID_PLAYER)
    {
        if (((id >= OBJID_START_OF_DMA_SPRITES) && (id <= OBJID_END_OF_DMA_SPRITES)) ||
            ((id >= OBJID_START_OF_DMA_LIGHT_SPRITES) && (id <= OBJID_END_OF_DMA_LIGHT_SPRITES)) )
        {
            uint16_t k = spr_get_vram_slot_16(i);
            
            if (k >= 128)
            {
                // Out of VRAM slots
                return -1;
            }
        }
    }

    struct game_object * p = &objects[i];

    p->id = id;
    p->uid = obj_get_uid();
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

        p->struct_data.npc_data.ani.display = (uint16_t)((uint32_t)ani_getframe_dynamic(p));
    }
    else
    {
        p->struct_data.npc_data.ani.display = ani_getframe_fixed_fast(p);
    }

    if ((id >= OBJID_START_OF_INTERACTABLES) && (id <= OBJID_END_OF_INTERACTABLES))
    {
        p->state = STATE_SWITCH_OFF;
        p->tile.x = p->pos.x.lh.h >> 4;
        p->tile.y = p->pos.y.lh.h >> 4;

        p->struct_data.interactable_data.event_flag = local_event_flag;
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

    if (id == OBJID_SLIME)
    {
        p->struct_data.npc_data.ani.last_address = 0;
        p->struct_data.npc_data.ani.last_dmafailed = 0;
        p->struct_data.npc_data.hp_tile_offset = 0;
        p->struct_data.npc_data.hp_display_time = 0;

        p->struct_data.npc_data.hp = ENEMY_HEALTH_STARTING;
        p->struct_data.npc_data.hp_max = ENEMY_HEALTH_STARTING;

        p->struct_data.npc_data.attack = ENEMY_ATTACK_VALUE;
        p->struct_data.npc_data.defense = ENEMY_DEFENSE_VALUE;

        p->struct_data.npc_data.hp_cache = ENEMY_HEALTH_STARTING;

        p->state = STATE_SPAWNING;
        p->struct_data.npc_data.status_time = 64 / V_MUL;

        uint8_t temp_weight = (uint8_t)rand_get16();
        p->struct_data.npc_data.money = (ENEMY_DROP_MONEY_MIN + (((ENEMY_DROP_MONEY_MAX - ENEMY_DROP_MONEY_MIN) * temp_weight) / 255));

        p->w = 16;
        p->h = 16;
    }

    if ((id == OBJID_DROP_MONEY) || (id == OBJID_DROP_REC_MEAT))
    {
        p->w = 16;
        p->h = 16;
    }

    obj_active_count++;

    if ((id == OBJID_FIREBALL) || (id == OBJID_HITBOX_INVISIBLE))
    {
        p->hit_type = 0x0001;

        p->w = 16;
        p->h = 16;
    }
    else if ((id == OBJID_HITBOX_INVISIBLE_E) || (id == OBJID_BUBBLE_E))
    {
        p->hit_type = 0x8001;

        p->w = 16;
        p->h = 16;
    }
    else
    {
        p->hit_type = 0x0000;
    }

    if (id == OBJID_INTERACTABLE_BLOCKER_FLOOR)
    {
        blocker_active_count++;
    }

    obj_set_function_pointer(p);

    return i;
}

/*
    Player hitboxes should call this instead
*/
int16_t obj_instantiate_hitbox_player(
    uint16_t id,
    int16_t x,
    int16_t y
)
{
    uint16_t i = hitbox_player_first_available;
    if (hitbox_player_first_available == 0xffff)
    {
        return -1;
    }
    hitbox_player_first_available = hitbox_player[i].next_free;

    uint16_t j = 0;

    // Player hitboxes don't need VRAM
    struct game_object * p = &hitbox_player[i];

    p->id = id;
    p->uid = obj_get_uid(); 
    p->array_index = i;
    
    p->pos.x.a = ((int32_t)x) << 16;
    p->pos.y.a = ((int32_t)y) << 16;

    p->struct_data.npc_data.ttl = 0; // always reset
    
    p->struct_data.npc_data.ani.frame = 0;

    p->pos.z.a = 0;
    p->delta.z.a = 0;

    p->struct_data.npc_data.ani.display = ani_getframe_fixed_fast(p);

    //obj_active_count++;
    hitbox_count_player++;

    p->hit_type = 0x0001;

    p->w = 16;
    p->h = 16;

    obj_set_function_pointer(p);

    return i;
}

uint16_t obj_instantiate_npcs(const struct obj_list_entry_spawns* list, int16_t offset_x, int16_t offset_y)
{
    while (list->id != OBJID_NULL)
    {
        if (obj_active_count >= OBJ_MAX_COUNT)
        {
            return 1;
        }

        uint16_t temp_objid = list->id;
        int16_t temp_x;
        int16_t temp_y;

        if (list->random_spread != 0)
        {
            int16_t temp_rand = (((int16_t)rand_get16()) % list->random_spread) - (list->random_spread >> 1);

            temp_x = list->x + temp_rand + offset_x;
            temp_y = list->y + temp_rand + offset_y;
        }
        else
        {
            temp_x = list->x + offset_x;
            temp_y = list->y + offset_y;
        }

        if (obj_instantiate(temp_objid, temp_x, temp_y, 0) == -1)
        {
            return 1;
        }

        list++;
    }

    return 0;
}

uint16_t obj_instantiate_spawners(const struct obj_list_entry_spawners* list)
{
    uint16_t temp_total_spawns = 0;

    while (list->id != OBJID_NULL)
    {
        if (obj_active_count >= OBJ_MAX_COUNT)
        {
            break;
        }

        uint16_t temp_objid = list->id;
        int16_t temp_x = list->x;
        int16_t temp_y = list->y;

        int16_t index = obj_instantiate(temp_objid, temp_x, temp_y, 0);

        if (index == -1)
        {
            break;
        }

        objects[index].struct_data.interactable_data.spawn_area_x = list->x;
        objects[index].struct_data.interactable_data.spawn_area_y = list->y;
        objects[index].struct_data.interactable_data.spawn_area_w = list->w;
        objects[index].struct_data.interactable_data.spawn_area_h = list->h;
        objects[index].struct_data.interactable_data.screen_x = list->screen_x;
        objects[index].struct_data.interactable_data.screen_y = list->screen_y;
        objects[index].struct_data.interactable_data.screen_w = list->screen_w;
        objects[index].struct_data.interactable_data.screen_h = list->screen_h;
        
        objects[index].data_ptr = (uint8_t *)list->spawn_list;

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
        obj_enemies_target_count = (uint16_t)((float)(obj_enemies_max_count) * 0.6f);

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

uint16_t obj_instantiate_interactables(const struct obj_list_entry_interactable* list)
{
    while (list->id != OBJID_NULL)
    {
        uint16_t temp_objid = list->id;
        int16_t temp_x = list->x;
        int16_t temp_y = list->y;
        uint16_t temp_flag = (uint16_t)((uint32_t)(list->flag));

        if (obj_active_count >= OBJ_MAX_COUNT)
        {
            return 1;
        }

        uint16_t index = obj_instantiate(temp_objid, temp_x, temp_y, temp_flag);
        if (index == -1)
        {
            return 1;
        }

        if (temp_objid == OBJID_INTERACTABLE_SIGN_WALL)
        {
            uint8_t * string_ptr = (uint8_t *)(list->flag);
            objects[index].data_ptr = string_ptr;
        }

        list++;
    }

    return 0;
}

/*! \def obj_destroy

    \brief Adds the object whose slot i into the deletion queue.
*/
inline void obj_destroy(uint16_t i)
{
    obj_delete_queue[obj_delete_queue_count] = i;

    obj_delete_queue_count++;

    return;
}

// Ditto for player hitboxes
inline void obj_destroy_hitbox_player(uint16_t i)
{
    hitbox_player_delete_queue[hitbox_player_delete_queue_count] = i;

    hitbox_player_delete_queue_count++;

    return;
}

/*! \def obj_cleanup

    \brief Deletes all objects whose slot i is within the deletion queue.
*/
void obj_cleanup()
{
    uint16_t temp_blocker_count = 0;

    for (int i = 0; i < obj_delete_queue_count; i++)
    {
        if (objects[obj_delete_queue[i]].id == OBJID_INTERACTABLE_BLOCKER_FLOOR)
        {
            temp_blocker_count++;
        }

        if (objects[obj_delete_queue[i]].id == OBJID_BUBBLE_E)
        {
            spr_release_vram_slot(obj_delete_queue[i], 1);
        }

        objects[obj_delete_queue[i]].id = OBJID_NULL;

        // Thread back the object to the next free
        objects[obj_delete_queue[i]].next_free = obj_first_available;
        obj_first_available = obj_delete_queue[i];

        // Fix the object function to dummy
        objects[obj_delete_queue[i]].func_ptr = (void *)&routines_dummy;
    }
    
    obj_active_count -= obj_delete_queue_count;
    blocker_active_count -= temp_blocker_count;

    obj_delete_queue_count = 0;
    return;
}

void obj_cleanup_hitbox_player()
{
    for (int i = 0; i < hitbox_player_delete_queue_count; i++)
    {
        hitbox_player[hitbox_player_delete_queue[i]].id = OBJID_NULL;

        // Thread back the object to the next free
        hitbox_player[hitbox_player_delete_queue[i]].next_free = hitbox_player_first_available;
        hitbox_player_first_available = hitbox_player_delete_queue[i];

        // Fix the object function to dummy
        hitbox_player[hitbox_player_delete_queue[i]].func_ptr = (void *)&routines_dummy;
    }
    
    hitbox_count_player -= hitbox_player_delete_queue_count;

    hitbox_player_delete_queue_count = 0;
    return;
}

inline uint16_t obj_get_uid()
{
    uint16_t temp_uid = obj_next_uid;
    obj_next_uid++;

    if (obj_next_uid == 0)
    {
        obj_next_uid++;
    }

    return temp_uid;
}

void obj_set_function_pointer(struct game_object * o)
{
    switch (o->id)
    {
        case OBJID_NULL:
            o->func_ptr = (void *)&routines_dummy;
            break;
        case OBJID_PLAYER:
            o->func_ptr = (void *)&routines_player;
            break;
        case OBJID_FIREBALL:
            o->func_ptr = (void *)&routines_fireball;
            break;
        case OBJID_SLIME:
            o->func_ptr = (void *)&routines_slime;
            break;
        case OBJID_BUBBLE_E:
            o->func_ptr = (void *)&routines_bubble_e;
            break;
        case OBJID_FX_SMOKE:
            o->func_ptr = (void *)&routines_fx_smoke;
            break;
        case OBJID_DROP_REC_MEAT:
            o->func_ptr = (void *)&routines_drop_rec_meat;
            break;
        case OBJID_DROP_MONEY:
            o->func_ptr = (void *)&routines_drop_money;
            break;
        case OBJID_HITBOX_INVISIBLE:
            o->func_ptr = (void *)&routines_hitbox_invis;
            break;
        case OBJID_HITBOX_INVISIBLE_E:
            o->func_ptr = (void *)&routines_hitbox_invis_e;
            break;
        case OBJID_SYS_IMPACT:
            o->func_ptr = (void *)&routines_fx_impact;
            break;
        case OBJID_INTERACTABLE_SWITCH_WALL:
        case OBJID_INTERACTABLE_SWITCH_FLOOR:
            o->func_ptr = (void *)&routines_interactable_switch;
            break;
        case OBJID_INTERACTABLE_BLOCKER_FLOOR:
        case OBJID_INTERACTABLE_BLOCKER_DOOR_NS:
        case OBJID_INTERACTABLE_BLOCKER_DOOR_EW:
            o->func_ptr = (void *)&routines_interactable_blocker;
            break;
        case OBJID_INTERACTABLE_SIGN_WALL:
            o->func_ptr = (void *)&routines_interactable_sign;
            break;
        case OBJID_SPAWNER_ENEMY:
            o->func_ptr = (void *)&routines_spawner;
            break;
        default:
            // Unimplemented object
            o->func_ptr = (void *)&routines_dummy;
            break;
    }

    return;
}

uint16_t move(struct game_object * o)
{
    // one axis needs to be tested at a time
    // if a collision check fails on an axis,
    // that axis' delta will be zeroed out

    // first test X axis movement
    int32_t temp_xl = o->pos.x.a + o->delta.x.a;
    
    int16_t temp_x = (int16_t)(temp_xl >> 16) + 1;
    int16_t temp_y = o->pos.y.lh.h + 1;
    
    // Get position tile
    if (o->delta.x.a > 0)
    {
        temp_x += 13;
    }

    int16_t temp_y_2 = temp_y + 13; // Lower edge

    // Needed for blocker tests
    int16_t temp_x_pixel = temp_x;

    temp_x >>= 4;
    temp_y >>= 4;
    temp_y_2 >>= 4;

    // Top edge.
    uint16_t temp_screen_x;
    uint16_t temp_screen_y;

    uint16_t temp_start_x;

    uint16_t temp_test_failed = 0;

    temp_start_x = (temp_x & 0xf);
    temp_screen_x = temp_x >> 4;

    uint16_t temp_start_y;

    temp_start_y = (temp_y & 0xf);
    temp_screen_y = temp_y >> 4;

    uint16_t temp_screen_offset;
    const uint8_t * q;

    temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

    q = map_current + 2 + temp_screen_offset + temp_start_x + ((temp_start_y) << 4);

    struct tile_xy t;
    t.x = temp_x;
    t.y = temp_y;
    

    if ((map_lut_col[*q] < 128) || hit_test_blocker(t))
    {
        o->delta.x.a = 0;
        temp_test_failed = 1;
    }

    // Bottom edge
    if (temp_test_failed != 1)
    {
        temp_start_y = (temp_y_2 & 0xf);
        temp_screen_y = temp_y_2 >> 4;

        temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

        q = map_current + 2 + temp_screen_offset + temp_start_x + ((temp_start_y) << 4);

        t.y = temp_y_2;

        if ((map_lut_col[*q] < 128) || (hit_test_blocker(t)))
        {
            o->delta.x.a = 0;
        }
    }

    // Additional check for screen bounds if it exists
    if (bg_scroll_x_bounds_min.full.high.a != -32768)
    {
        if (temp_x_pixel < bg_scroll_x_bounds_min.full.high.a)
        {
            o->delta.x.a = 0;
            temp_test_failed = 1;
        }
        else if (temp_x_pixel > bg_scroll_x_bounds_max.full.high.a + 256)
        {
            o->delta.x.a = 0;
            temp_test_failed = 1;
        }
    }
    
    // Now for the top edge Y axis moves
    int32_t temp_yl = o->pos.y.a + o->delta.y.a;
    
    temp_x = o->pos.x.lh.h + 1;
    temp_y = (int16_t)(temp_yl >> 16) + 1;
    
    // Get position tile
    if (o->delta.y.a > 0)
    {
        temp_y += 13;
    }

    int16_t temp_y_pixel = temp_y;
    
    int16_t temp_x_2 = temp_x + 13; // right edge

    temp_x >>= 4;
    temp_y >>= 4;
    temp_x_2 >>= 4;

    temp_test_failed = 0;

    // Left edge.
    temp_start_x = (temp_x & 0xf);
    temp_screen_x = temp_x >> 4;

    temp_start_y = (temp_y & 0xf);
    temp_screen_y = temp_y >> 4;

    temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

    q = map_current + 2 + temp_screen_offset + temp_start_x + ((temp_start_y) << 4);

    t.x = temp_x;
    t.y = temp_y;

    if ((map_lut_col[*q] < 128) || (hit_test_blocker(t)))
    {
        o->delta.y.a = 0;
        if ((o->delta.x.a | o->delta.y.a) == 0)
        {
            return 1;
        }
        temp_test_failed = 1;
    }

    // Right edge
    if (temp_test_failed != 1)
    {
        temp_start_x = (temp_x_2 & 0xf);
        temp_screen_x = temp_x_2 >> 4;

        temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

        q = map_current + 2 + temp_screen_offset + temp_start_x + ((temp_start_y) << 4);

        t.x = temp_x_2;
        
        if ((map_lut_col[*q] >= 128) || (hit_test_blocker(t)))
        {
            // One final check if a bounding box exists
            if (bg_scroll_y_bounds_min.full.high.a != -32768)
            {
                if (temp_y_pixel < bg_scroll_y_bounds_min.full.high.a)
                {
                    o->delta.y.a = 0;
                }
                else if (temp_y_pixel > bg_scroll_y_bounds_max.full.high.a + 224)
                {
                    o->delta.y.a = 0;
                }

                if ((o->delta.x.a | o->delta.y.a) == 0)
                {
                    return 1;
                }
            }

            o->pos.y.a += o->delta.y.a; // only change Y position if all Y tests pass
        }
        else
        {
            o->delta.y.a = 0;
            if ((o->delta.x.a | o->delta.y.a) == 0)
            {
                return 1;
            }
        }
    }

    // X must be done as the final calculation
    o->pos.x.a += o->delta.x.a; // Always saved

    return 0;
}

void move_nocol_fast(struct game_object * o)
{
    // Move an object ignoring everything
    // Useful for light objects that do not need to test anything.
    // optionally also ignoring map edge

    o->pos.x.a += o->delta.x.a;
    o->pos.y.a += o->delta.y.a;

    return;
}

/*
    Returns the squared distance (avoid a square root)

    Limited to max 320 on either axis to prevent the LUT from going too large.
*/
uint32_t ai_distance_squared(int16_t x, int16_t y)
{
    // c^2 = a^2 + b^2
    uint16_t abs_y = (y < 0) ? -y : y;
    uint16_t abs_x = (x < 0) ? -x : x;

    if (abs_x > 320)
    {
        abs_x = 320;
    }

    if (abs_y > 320)
    {
        abs_y = 320;
    }

    return (data_pow_2[abs_x] + data_pow_2[abs_y]);
}

/*
    Gameplay AI for non-player entities
*/
uint16_t ai_run(struct game_object * o, uint32_t dist, int16_t x, int16_t y)
{
    uint16_t temp_invalidate_animation_frame = 0;
    uint16_t temp_interrupted = 0;

    // Do some AI interruption given some conditions
    if (
        (((dist <= DIST_MELEE) && o->struct_data.npc_data.ai_state != AI_STATE_IDLE) ||
        (o->struct_data.npc_data.status == STATUS_BURNING)) && (o->struct_data.npc_data.ai_state != AI_STATE_ATTACK)
        )
    {
        // If AI gets too close to the player AND the AI isn't idling (i.e. moving)
        // OR if the AI is on fire, HOWEVER
        // Do not let the AI get interrupted if mid-attack
        o->struct_data.npc_data.ai_timer = 0;
        temp_interrupted = 1;
    }

    if (o->struct_data.npc_data.ai_timer == 0) // Only process AI state changes when this timer hits 0
    {
        uint8_t temp_rand = (uint8_t)rand_get16();
        
        // If the AI is idling, attacked the player, or gets interrupted by the player while not attacking...
        if (o->struct_data.npc_data.ai_state == AI_STATE_IDLE || (temp_interrupted && (o->struct_data.npc_data.ai_state != AI_STATE_ATTACK)))
        {
            // Object has finished idling, time to move once more.
            // Check if a minimum distance is met or not
            if ((dist > DIST_MELEE) && (o->struct_data.npc_data.status != STATUS_BURNING))
            {
                // Enemy is far away from player
                uint8_t temp_angle = atan2_uint8(y, x) + (temp_rand & 0x0f) - 8;

                o->angle = temp_angle;

                o->delta.x.a = data_cosine_1[temp_angle] * V_MUL;
                o->delta.y.a = data_sine_1[temp_angle] * V_MUL;

                o->struct_data.npc_data.ai_state = AI_STATE_MOVE_TOWARDS;
                if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
                {
                    o->state = STATE_HURT_BURN_MOVE;
                }
                else
                {
                    o->state = STATE_MOVE_WALK;
                }

                o->facing = ai_get_facing(o);

                temp_invalidate_animation_frame = 1;
            }
            else if (o->struct_data.npc_data.ai_state != AI_STATE_MOVE_AWAY) // don't recalc for anything already moving away.
            {
                // Enemy is close to player
                uint8_t temp_angle = (atan2_uint8(y, x)) + (temp_rand & 0x0f) - 136;

                o->angle = temp_angle;

                o->delta.x.a = data_cosine_1[temp_angle] * V_MUL;
                o->delta.y.a = data_sine_1[temp_angle] * V_MUL;

                o->struct_data.npc_data.ai_state = AI_STATE_MOVE_AWAY;

                if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
                {
                    o->state = STATE_HURT_BURN_MOVE;
                }
                else
                {
                    o->state = STATE_MOVE_WALK;
                }

                o->facing = ai_get_facing(o);

                temp_invalidate_animation_frame = 1;
            }

            o->struct_data.npc_data.ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;
        }
        else if ((o->struct_data.npc_data.ai_state == AI_STATE_MOVE_TOWARDS) || (o->struct_data.npc_data.ai_state == AI_STATE_MOVE_AWAY))
        {
            // Object has finished its movement time and should decide on a possible action.
            o->delta.x.a = 0;
            o->delta.y.a = 0;

            if (o->struct_data.npc_data.ai_state == AI_STATE_MOVE_AWAY)
            {
                o->angle = (o->angle) + 128;
                o->facing = ai_get_facing(o);
            }

            // Now to decide if the AI is going to attack or not.
            if (temp_rand >= 128)
            {
                // around half chance of performing an attack.
                // During this situation, check the previous state
                // to pick the correct attack for the range.

                if (o->struct_data.npc_data.ai_state == AI_STATE_MOVE_TOWARDS)
                {
                    o->state = STATE_ATTACK_BASIC;
                }
                else
                {
                    o->state = STATE_ATTACK_SPECIAL; 
                }

                // For now, duplicate of the other case.
                o->struct_data.npc_data.ai_state = AI_STATE_ATTACK;
                
                o->struct_data.npc_data.ai_timer = (15) / V_MUL;

                o->struct_data.npc_data.ai_makeattack = 1;
            }
            else
            {
                // Not attacking. Fix the state so the animation makes sense.
                o->struct_data.npc_data.ai_state = AI_STATE_IDLE;

                if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
                {
                    o->state = STATE_HURT_BURN;
                }
                else
                {
                    o->state = STATE_IDLE;
                }

                o->struct_data.npc_data.ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;
            } 

            temp_invalidate_animation_frame = 1;
        }
        else if (o->struct_data.npc_data.ai_state == AI_STATE_ATTACK)
        {
            // Reset the AI to idle
            o->struct_data.npc_data.ai_state = AI_STATE_IDLE;

            if ((o->state == STATE_HURT_BURN || o->state == STATE_HURT_BURN_MOVE))
            {
                o->state = STATE_HURT_BURN;
            }
            else
            {
                o->state = STATE_IDLE;
            }

            o->struct_data.npc_data.ai_timer = (30 + (temp_rand & 0x1f)) / V_MUL;

            temp_invalidate_animation_frame = 1;
        }
    }

    ai_idle(o); // tick down the timer

    return temp_invalidate_animation_frame;
}

inline void ai_idle(struct game_object * o)
{
    o->struct_data.npc_data.ai_timer--;

    return;
}

inline uint16_t ai_get_facing(struct game_object * o)
{
    // Adjust the facing based on angle.
    if (o->angle < 32)
    {
        return FACING_RIGHT;
    }
    else if (o->angle < 96)
    {
        return FACING_DOWN;
    }
    else if (o->angle < 160)
    {
        return FACING_LEFT;
    }
    else if (o->angle < 224)
    {
        return FACING_UP;
    }
    else
    {
        return FACING_RIGHT;
    }
}
