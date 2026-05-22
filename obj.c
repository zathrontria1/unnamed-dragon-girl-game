#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "routines.h"
#include "routines_player.h"
#include "routines_enemy.h"
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

void obj_run() 
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
    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
	    "\tx16\n"

        "\tphy\n"

        "\tldx #<_obj_general\n"
        "\tldy #0\n"
        "\tcpy _obj_active_count\n"
        "\tbcs .object_break_loop\n"

        ".object_process_loop:\n"
            "\tlda $7e0000,x\n" // assumes object memory is in bank 7e
            "\tbeq .object_process_increment\n"
            "\tiny\n"
            "\tlda $7e0037,x\n"
            "\tsta _system_JMLCodeInWRAM+2\n"
            "\tlda $7e0036,x\n"
            "\tsta _system_JMLCodeInWRAM+1\n"
            "\tphy\n"
            "\tphx\n"
            "\ttxa\n"
            "\tldx #^_obj_general\n"
            "\tjsl >_system_JMLCodeInWRAM\n"
            "\tplx\n"
            "\tply\n"

        ".object_process_increment:\n"
            "\ttxa\n"
            "\tclc\n"
            "\tadc #128\n"
            "\tcmp #<_obj_general+5120\n"
            "\tbcs .object_break_loop\n"
            "\ttax\n"
            "\tcpy _obj_active_count\n"
            "\tbcc .object_process_loop\n"

        ".object_break_loop:\n"
            "\tply\n"
    );
    #else
        int obj_process_count = 0;

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
            obj_process_count++;

            ptr++;

            if (obj_process_count >= obj_active_count)
            {
                break;
            }
        }
    #endif

    // Repeat for player hitboxes
    obj_player_active_fireballs = 0;

    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
	    "\tx16\n"

        "\tphy\n"
        
        "\tldx #<_obj_hitbox_player\n"
        "\tlda _obj_hitbox_count_player\n"
        "\tbeq .hitbox_player_break_loop\n"
        
        ".hitbox_player_process_loop:\n"
            "\tlda $7e0037,x\n"
            "\tsta _system_JMLCodeInWRAM+2\n"
            "\tlda $7e0036,x\n"
            "\tsta _system_JMLCodeInWRAM+1\n"
            "\tphx\n"
            "\ttxa\n"
            "\tldx #^_obj_hitbox_player\n"
            "\tjsl >_system_JMLCodeInWRAM\n"
            "\tplx\n"
        ".hitbox_player_process_increment:\n"
            "\ttxa\n"
            "\tclc\n"
            "\tadc #128\n"
            "\ttax\n"
            "\tcpx #<_obj_hitbox_player+2048\n"
            "\tbcc .hitbox_player_process_loop\n"

        ".hitbox_player_break_loop:\n"
            "\tply\n"
    );
    #else
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
        
    #endif

    if (obj_player_active_fireballs > 0)
    {
        Gfx_SetColorMath(obj_player_active_fireballs,0,0);
        gfx_cmath_change = -64 * V_MUL;
        shadow_cgwsub = 0x00;
        shadow_cgadsub = 0x32;
    }

    // Finally repeat for enemies
    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
	    "\tx16\n"

        "\tphy\n"
        
        "\tldx #<_obj_hitbox_enemy\n"

        "\tlda _obj_hitbox_count_enemy\n"
        "\tbeq .hitbox_enemy_break_loop\n"

        ".hitbox_enemy_process_loop:\n"
            "\tlda $7e0037,x\n"
            "\tsta _system_JMLCodeInWRAM+2\n"
            "\tlda $7e0036,x\n"
            "\tsta _system_JMLCodeInWRAM+1\n"
            "\tphx\n"
            "\ttxa\n"
            "\tldx #^_obj_hitbox_enemy\n"
            "\tjsl >_system_JMLCodeInWRAM\n"
            "\tplx\n"

        ".hitbox_enemy_process_increment:\n"
            "\ttxa\n"
            "\tclc\n"
            "\tadc #128\n"
            "\ttax\n"
            "\tcpx #<_obj_hitbox_enemy+1024\n"
            "\tbcc .hitbox_enemy_process_loop\n"

        ".hitbox_enemy_break_loop:\n"
            "\tply\n"
    );
    #else
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
    #endif

    event_in_combat_shadow = event_in_combat;

    if ((event_in_combat == 0) && 
    (!system_game_paused) &&
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
            SoundInterface_StopSfx(SFX_ATK_FIRE_BREATH);
            snd_flame_playing = 0;
        }
    }

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

            "\tphy\n"
            "\tphb\n"

            "\tpei (r0)\n"
            "\tpei (r1)\n"

            "\txba\n" 
            "\tlsr\n" // mul 128
            "\ttax\n"
            "\tsta r0\n" // offset of start, also subtract length with this
            "\tlda #5120\n"
            "\tsec\n"
            "\tsbc r0\n"
            "\tsta r1\n" // actual transfer length

            "\ta8\n"
            "\tsep #$20\n"
            "\tlda #$00\n"
            "\tsta >_obj_general,x\n" // write first zero byte
            "\tlda #^_obj_general\n"
            "\tsta >_system_MVNCodeInWRAM+1\n" // write bank byte of source 
            "\tsta >_system_MVNCodeInWRAM+2\n" // ditto for destination

            "\ta16\n"
            "\trep #$21\n"
            "\tlda r0\n" // load source address
            "\tadc #<_obj_general\n" 
            "\ttax\n" 
            "\ttay\n" 
            "\tiny\n" // destination address
            "\tlda r1\n" // load transfer length
            "\tdec\n" 
            "\tdec\n" // Decrement by 2 to remove the first byte and MVN implied byte
            "\tjsl >_system_MVNCodeInWRAM;\n"

            "\tply\n"
            "\tsty r1\n"
            "\tply\n"
            "\tsty r0\n"

            "\tplb\n"
            "\tply\n"
        );
    #else
        uint8_t * ptr = (uint8_t *)&obj_general[start_index];
        for (int i = start_index; i < (OBJ_GENERAL_MAX_COUNT * (uint16_t)sizeof(struct game_object)); i++)
        {
            *ptr = 0x00;
            ptr++;
        }
    #endif

    // Then initialize the next pointers and function pointers for all of them
    for (int i = start_index; i < (OBJ_GENERAL_MAX_COUNT - 1); i++)
    {
        obj_general[i].next_free = i+1;
        obj_general[i].func_ptr = (void *)&routines_dummy;
    }

    obj_general[OBJ_GENERAL_MAX_COUNT - 1].next_free = 0xffff;
    obj_general[OBJ_GENERAL_MAX_COUNT - 1].func_ptr = (void *)&routines_dummy;

    obj_first_available = 0;

    return;
}

/*
    Player hitboxes (e.g. fireballs) reset too, in a separate list for performance reasons
*/
void obj_reset_hitbox_player()
{
    #if VBCC_ASM == 1 // Write the first byte as zero, then use MVN to copy the rest.
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\tphy\n"
            "\tphb\n"

            "\tpei (r0)\n"
            "\tpei (r1)\n"

            "\tstz r0\n" // offset of start, also subtract length with this
            "\tlda #2048\n"
            "\tsec\n"
            "\tsbc r0\n"
            "\tsta r1\n" // actual transfer length

            "\ta8\n"
            "\tsep #$20\n"
            "\tlda #$00\n"
            "\tsta >_obj_hitbox_player\n" // write first zero byte
            "\tlda #^_obj_hitbox_player\n"
            "\tsta >_system_MVNCodeInWRAM+1\n" // write bank byte of source 
            "\tsta >_system_MVNCodeInWRAM+2\n" // ditto for destination

            "\ta16\n"
            "\trep #$21\n"
            "\tlda r0\n" // load source address
            "\tadc #<_obj_hitbox_player\n" 
            "\ttax\n" 
            "\ttay\n" 
            "\tiny\n" // destination address
            "\tlda r1\n" // load transfer length
            "\tdec\n" 
            "\tdec\n" // Decrement by 2 to remove the first byte and MVN implied byte
            "\tjsl >_system_MVNCodeInWRAM;\n"

            "\tply\n"
            "\tsty r1\n"
            "\tply\n"
            "\tsty r0\n"

            "\tplb\n"
            "\tply\n");
    #else
        uint8_t * ptr = (uint8_t *)&obj_hitbox_player[0];
        for (int i = 0; i < (OBJ_PLAYERHITBOX_MAX_COUNT * (uint16_t)sizeof(struct game_object)); i++)
        {
            *ptr = 0x00;
            ptr++;
        }
    #endif

    // Then initialize the next pointers and function pointers for all of them
    for (int i = 0; i < (OBJ_PLAYERHITBOX_MAX_COUNT - 1); i++)
    {
        obj_hitbox_player[i].next_free = i+1;
        obj_hitbox_player[i].func_ptr = (void *)&routines_dummy;
    }

    obj_hitbox_player[OBJ_PLAYERHITBOX_MAX_COUNT - 1].next_free = 0xffff;
    obj_hitbox_player[OBJ_PLAYERHITBOX_MAX_COUNT - 1].func_ptr = (void *)&routines_dummy;

    obj_hitbox_player_first_available = 0;

    return;
}

/*
    Do the same for enemy hitboxes too
*/
void obj_reset_hitbox_enemy()
{
    #if VBCC_ASM == 1 // Write the first byte as zero, then use MVN to copy the rest.
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\tphy\n"
            "\tphb\n"

            "\tpei (r0)\n"
            "\tpei (r1)\n"

            "\tstz r0\n" // offset of start, also subtract length with this
            "\tlda #1024\n"
            "\tsec\n"
            "\tsbc r0\n"
            "\tsta r1\n" // actual transfer length

            "\ta8\n"
            "\tsep #$20\n"
            "\tlda #$00\n"
            "\tsta >_obj_hitbox_enemy\n" // write first zero byte
            "\tlda #^_obj_hitbox_enemy\n"
            "\tsta >_system_MVNCodeInWRAM+1\n" // write bank byte of source 
            "\tsta >_system_MVNCodeInWRAM+2\n" // ditto for destination

            "\ta16\n"
            "\trep #$21\n"
            "\tlda r0\n" // load source address
            "\tadc #<_obj_hitbox_enemy\n" 
            "\ttax\n" 
            "\ttay\n" 
            "\tiny\n" // destination address
            "\tlda r1\n" // load transfer length
            "\tdec\n" 
            "\tdec\n" // Decrement by 2 to remove the first byte and MVN implied byte
            "\tjsl >_system_MVNCodeInWRAM;\n"

            "\tply\n"
            "\tsty r1\n"
            "\tply\n"
            "\tsty r0\n"

            "\tplb\n"
            "\tply\n");
    #else
        uint8_t * ptr = (uint8_t *)&obj_hitbox_enemy[0];

        for (int i = 0; i < (OBJ_ENEMYHITBOX_MAX_COUNT * (uint16_t)sizeof(struct game_object)); i++)
        {
            *ptr = 0x00;
            ptr++;
        }
    #endif

    // Then initialize the next pointers and function pointers for all of them
    for (int i = 0; i < (OBJ_ENEMYHITBOX_MAX_COUNT - 1); i++)
    {
        obj_hitbox_enemy[i].next_free = i+1;
        obj_hitbox_enemy[i].func_ptr = (void *)&routines_dummy;
    }

    obj_hitbox_enemy[OBJ_ENEMYHITBOX_MAX_COUNT - 1].next_free = 0xffff;
    obj_hitbox_enemy[OBJ_ENEMYHITBOX_MAX_COUNT - 1].func_ptr = (void *)&routines_dummy;

    obj_hitbox_enemy_first_available = 0;

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
    obj_first_available = obj_general[i].next_free;

    uint16_t j = 0;
    // perform additional checks for sprite slots

    if (id != OBJID_PLAYER)
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
        p->tile.x = (uint16_t)p->pos.x.lh.h >> 4;
        p->tile.y = (uint16_t)p->pos.y.lh.h >> 4;

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

    // Send the ID to a helper function to set the correct data.
    // If the function returns false, this is not an 
    // enemy and set things
    if (obj_get_enemy_data(p))
    {
        p->struct_data.npc_data.ani.last_address = 0;
        p->struct_data.npc_data.ani.last_dmafailed = 0;
        p->struct_data.npc_data.hp_tile_offset = 0;
        p->struct_data.npc_data.hp_display_time = 0;

        p->state = STATE_SPAWNING;
        p->struct_data.npc_data.status_time = 64 / V_MUL;
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

        if ((id == OBJID_INTERACTABLE_BLOCKER_FLOOR) || 
            (id == OBJID_INTERACTABLE_BLOCKER_DOOR_EW) || 
            (id == OBJID_INTERACTABLE_BLOCKER_DOOR_NS))
        {
            // Set a non-zero value
            p->struct_data.interactable_data.delay_time = 0xffff;
        }
    }

    obj_active_count++;

    obj_set_function_pointer(p);

    p->r = p->pos.x.lh.h + p->w;
    p->b = p->pos.y.lh.h + p->h;

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
    obj_hitbox_count_player++;

    p->hit_type = 0x0001;

    p->w = 16;
    p->h = 16;

    obj_set_function_pointer(p);

    p->r = p->pos.x.lh.h + p->w;
    p->b = p->pos.y.lh.h + p->h;

    return i;
}

/*
    And use this for enemy hitboxes
*/
int16_t obj_instantiate_hitbox_enemy(
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
    obj_hitbox_enemy_first_available = obj_hitbox_enemy[i].next_free;

    uint16_t j = 0;

    // perform additional checks for sprite slots
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

        obj_hitbox_enemy[i].struct_data.npc_data.tilenum = temp_tilenum;
        obj_hitbox_enemy[i].struct_data.npc_data.vram_addr = temp_tilenum << 4; // mul by 16, size of a 8x8 tile - this is in words
    }

    struct game_object * p = &obj_hitbox_enemy[i];

    p->id = id;
    p->uid = obj_get_uid(); 
    p->array_index = i;
    
    p->pos.x.a = ((int32_t)x) << 16;
    p->pos.y.a = ((int32_t)y) << 16;

    p->struct_data.npc_data.ttl = 0; // always reset
    
    p->struct_data.npc_data.ani.frame = 0;

    p->pos.z.a = 0;
    p->delta.z.a = 0;

    p->state = STATE_IDLE;
    p->facing = FACING_DOWN;
    p->struct_data.npc_data.ani.display = (uint16_t)((uint32_t)ani_getframe_dynamic(p));

    obj_hitbox_count_enemy++;

    p->hit_type = 0x8001;

    p->w = 16;
    p->h = 16;

    obj_set_function_pointer(p);

    p->r = p->pos.x.lh.h + p->w;
    p->b = p->pos.y.lh.h + p->h;

    return i;
}

uint16_t obj_instantiate_npcs(const struct obj_list_entry_spawns* list, int16_t offset_x, int16_t offset_y)
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
        if (obj_active_count >= OBJ_GENERAL_MAX_COUNT)
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

        if (obj_active_count >= OBJ_GENERAL_MAX_COUNT)
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
            obj_general[index].data_ptr = string_ptr;
        }

        list++;
    }

    return 0;
}

/*! \def obj_destroy

    \brief Adds the object whose slot i into the deletion queue.
*/
FORCE_INLINE void obj_destroy(uint16_t i)
{
    obj_delete_queue[obj_delete_queue_count] = i;

    obj_delete_queue_count++;

    return;
}

// Ditto for player hitboxes
FORCE_INLINE void obj_destroy_hitbox_player(uint16_t i)
{
    obj_hitbox_player_delete_queue[obj_hitbox_player_delete_queue_count] = i;

    obj_hitbox_player_delete_queue_count++;

    return;
}

// Lastly for enemy hitboxes
FORCE_INLINE void obj_destroy_hitbox_enemy(uint16_t i)
{
    obj_hitbox_enemy_delete_queue[obj_hitbox_enemy_delete_queue_count] = i;

    obj_hitbox_enemy_delete_queue_count++;

    return;
}

/*! \def obj_cleanup

    \brief Deletes all objects whose slot i is within the deletion queue.
*/
void obj_cleanup()
{
    for (int i = 0; i < obj_delete_queue_count; i++)
    {
        if (obj_general[obj_delete_queue[i]].id == OBJID_SLIME)
        {
            SpriteEngine_ReleaseVramSlot(obj_delete_queue[i], 1);
        }

        obj_general[obj_delete_queue[i]].id = OBJID_NULL;

        // Thread back the object to the next free
        obj_general[obj_delete_queue[i]].next_free = obj_first_available;
        obj_first_available = obj_delete_queue[i];

        // Fix the object function to dummy
        obj_general[obj_delete_queue[i]].func_ptr = (void *)&routines_dummy;
    }
    
    obj_active_count -= obj_delete_queue_count;

    obj_delete_queue_count = 0;
    return;
}

void obj_cleanup_hitbox_player()
{
    for (int i = 0; i < obj_hitbox_player_delete_queue_count; i++)
    {
        obj_hitbox_player[obj_hitbox_player_delete_queue[i]].id = OBJID_NULL;

        // Thread back the object to the next free
        obj_hitbox_player[obj_hitbox_player_delete_queue[i]].next_free = obj_hitbox_player_first_available;
        obj_hitbox_player_first_available = obj_hitbox_player_delete_queue[i];

        // Fix the object function to dummy
        obj_hitbox_player[obj_hitbox_player_delete_queue[i]].func_ptr = (void *)&routines_dummy;
    }
    
    obj_hitbox_count_player -= obj_hitbox_player_delete_queue_count;

    obj_hitbox_player_delete_queue_count = 0;
    return;
}

void obj_cleanup_hitbox_enemy()
{
    for (int i = 0; i < obj_hitbox_enemy_delete_queue_count; i++)
    {
        if (obj_hitbox_enemy[obj_hitbox_enemy_delete_queue[i]].id == OBJID_BUBBLE_E)
        {
            SpriteEngine_ReleaseVramSlot(obj_hitbox_enemy_delete_queue[i], 1);
        }

        obj_hitbox_enemy[obj_hitbox_enemy_delete_queue[i]].id = OBJID_NULL;

        // Thread back the object to the next free
        obj_hitbox_enemy[obj_hitbox_enemy_delete_queue[i]].next_free = obj_hitbox_enemy_first_available;
        obj_hitbox_enemy_first_available = obj_hitbox_enemy_delete_queue[i];

        // Fix the object function to dummy
        obj_hitbox_enemy[obj_hitbox_enemy_delete_queue[i]].func_ptr = (void *)&routines_dummy;
    }
    
    obj_hitbox_count_enemy -= obj_hitbox_enemy_delete_queue_count;

    obj_hitbox_enemy_delete_queue_count = 0;
    return;
}

FORCE_INLINE uint16_t obj_get_uid()
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

bool obj_get_enemy_data(struct game_object * o)
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