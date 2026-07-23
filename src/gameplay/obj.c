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

#include "crash_handler.h"

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

#if defined(PROFILE_OBJECT_CALLS) || defined(DEBUG_ALL)
volatile uint16_t obj_profile_dispatch_barrier;

NO_INLINE void ObjectSystem_ProcessObjectProfiled(struct game_object * o)
{
    switch (o->id)
    {
        case OBJID_SYS_IMPACT:
            Routines_Fx_Impact(o);
            break;
        case OBJID_FX_SMOKE:
            Routines_Fx_Smoke(o);
            break;
        case OBJID_HITBOX_INVISIBLE:
            Routines_Player_InvisibleHit(o);
            break;
        case OBJID_HITBOX_INVISIBLE_E:
            Routines_Enemy_InvisibleHit(o);
            break;
        case OBJID_FIREBALL:
            Routines_Player_Fireball(o);
            break;
        case OBJID_BUBBLE_E:
            Routines_Enemy_Slime_Bubble(o);
            break;
        case OBJID_ARROW_E:
            Routines_Enemy_Lizardman_Arrow(o);
            break;
        case OBJID_DROP_MONEY:
            Routines_Drops_Money(o);
            break;
        case OBJID_DROP_REC_MEAT:
            Routines_Drops_Recovery_Meat(o);
            break;
        case OBJID_SLIME:
            Routines_Enemy_Slime(o);
            break;
        case OBJID_LIZARDMAN:
            Routines_Enemy_Lizardman(o);
            break;
        case OBJID_PLAYER:
            Routines_Player(o);
            break;
        case OBJID_BOSS_TEST1:
            Routines_Boss_Test(o);
            break;
        case OBJID_BOSS_TEST1_ATTACK1:
            Routines_Boss_Test_Attack_Particle(o);
            break;
        case OBJID_INTERACTABLE_SWITCH_WALL:
        case OBJID_INTERACTABLE_SWITCH_FLOOR:
            Routines_Interactables_Switch(o);
            break;
        case OBJID_INTERACTABLE_BLOCKER_FLOOR:
        case OBJID_INTERACTABLE_BLOCKER_DOOR_NS:
        case OBJID_INTERACTABLE_BLOCKER_DOOR_EW:
            Routines_Interactable_Blocker(o);
            break;
        case OBJID_INTERACTABLE_LEVEL_WARP:
            Routines_LevelWarp(o);
            break;
        case OBJID_INTERACTABLE_SIGN_WALL:
            Routines_Interactable_Sign(o);
            break;
        case OBJID_SPAWNER_ENEMY:
            Routines_EnemySpawner(o);
            break;
        case OBJID_INTERACTABLE_TREASURECHEST:
            Routines_TreasureChest(o);
            break;
        case OBJID_NULL:
        default:
            Routines_Dummy(o);
            break;
    }

    obj_profile_dispatch_barrier = o->id; // Prevent compiler from optimizing out the function call
}
#endif

// A generation number used to mark the small set of enemies allowed to run
// their expensive decision-making routine this frame. An enemy is scheduled
// when its stored token matches this value; advancing the generation makes
// every mark from the previous frame stale without clearing the whole pool.
static uint32_t obj_enemy_ai_schedule_token;

// Start each scan after the previous scan's stopping point so one object near
// the start of the pool cannot monopolize the per-frame AI budget.
static uint16_t obj_enemy_ai_next_index;

/**
 * Selects nearby, active enemies for full AI processing this frame.
 *
 * Enemy routines still run for every visible enemy, but only enemies marked
 * with the current schedule token call Routines_Enemy_Ai_Process(). The scan
 * is capped by ENEMY_AI_MAX_PER_FRAME and wraps once around the object pool.
 */
void ObjectSystem_ScheduleEnemyAi(void)
{
    // Start a new generation before marking any objects. Old marks then fail
    // ObjectSystem_IsEnemyAiScheduled() automatically.
    obj_enemy_ai_schedule_token++;
    if (obj_enemy_ai_schedule_token == 0xffffffffl)
    {
        // Avoid using the sentinel value assigned to newly created objects.
        obj_enemy_ai_schedule_token = 0;
    }

    // Object processing can begin before the player has been instantiated.
    if (obj_player_pointer == 0)
    {
        return;
    }

    uint16_t selected = 0;
    uint16_t index = obj_enemy_ai_next_index;

    // Check at most one complete pass. This prevents an empty or sparse pool
    // from making the scheduler loop indefinitely while it seeks a target.
    for (uint16_t checked = 0; checked < OBJ_GENERAL_MAX_COUNT && selected < ENEMY_AI_MAX_PER_FRAME; checked++)
    {
        struct game_object * o = &obj_general[index];

        // Spawning and dying enemies are handled by their state transitions;
        // only normal slime/lizardman objects are candidates for AI work.
        if ((o->id == OBJID_SLIME || o->id == OBJID_LIZARDMAN) &&
            o->state != STATE_DIE && o->state != STATE_SPAWNING)
        {
            int16_t x = o->pos.x.lh.h - obj_player_pointer->pos.x.lh.h;
            int16_t y = o->pos.y.lh.h - obj_player_pointer->pos.y.lh.h;

            // Use squared distance to avoid a square root. The same range is
            // checked by the enemy update routine before consuming this mark.
            if (Math_GetDistanceSquared(x, y) < DIST_AI_MAX)
            {
                o->struct_data.npc_data.ai_scheduled_token = obj_enemy_ai_schedule_token;
                selected++;
            }
        }

        index++;
        if (index >= OBJ_GENERAL_MAX_COUNT)
        {
            index = 0;
        }
    }

    // Resume at the first unchecked slot next frame. If the budget was met,
    // this is immediately after the selected enemy; otherwise it is where the
    // complete pass ended, which preserves the round-robin behavior.
    obj_enemy_ai_next_index = index;
}

bool ObjectSystem_IsEnemyAiScheduled(const struct game_object * o)
{
    // Token comparison gives O(1) membership testing without clearing every
    // NPC's scheduling field at the start of each frame.
    return o->struct_data.npc_data.ai_scheduled_token == obj_enemy_ai_schedule_token;
}

/**
 * @brief Main object engine processing loop.
 * 
 * Executes logic ticks, updates hitboxes, and processes object deletion queues.
 */
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

    if (!system_game_paused)
    {
        ObjectSystem_ScheduleEnemyAi();
    }

    // New implementation
    struct game_object * ptr = (struct game_object *)&obj_general[0];

    if (obj_active_count != 0)
    {
        uint16_t processed = 0;
        for (int i = 0; i < OBJ_GENERAL_MAX_COUNT; i++)
        {
            if (ptr->id != OBJID_NULL)
            {
#if defined(PROFILE_OBJECT_CALLS) || defined(DEBUG_ALL)
                ObjectSystem_ProcessObjectProfiled(ptr);
#else
                void (*func)(struct game_object *) = ptr->func_ptr;
                func(ptr);
#endif
                if (map_tilemap_recovery_pending)
                {
                    return;
                }
                processed++;
                if (processed >= obj_active_count)
                {
                    break;
                }
            }
            ptr++;
        }
    }

    // Repeat for player hitboxes
    obj_player_active_fireballs = 0;

    ptr = (struct game_object *)&obj_hitbox_player[0];

    if(obj_hitbox_count_player != 0)
    {
        uint16_t processed = 0;
        for (int i = 0; i < OBJ_PLAYERHITBOX_MAX_COUNT; i++)
        {
            if (ptr->id != OBJID_NULL)
            {
#if defined(PROFILE_OBJECT_CALLS) || defined(DEBUG_ALL)
                ObjectSystem_ProcessObjectProfiled(ptr);
#else
                void (*func)(struct game_object *) = ptr->func_ptr;
                func(ptr);
#endif
                if (map_tilemap_recovery_pending)
                {
                    return;
                }
                processed++;
                if (processed >= obj_hitbox_count_player)
                {
                    break;
                }
            }
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
        uint16_t processed = 0;
        for (int i = 0; i < OBJ_ENEMYHITBOX_MAX_COUNT; i++)
        {
            if (ptr->id != OBJID_NULL)
            {
#if defined(PROFILE_OBJECT_CALLS) || defined(DEBUG_ALL)
                ObjectSystem_ProcessObjectProfiled(ptr);
#else
                void (*func)(struct game_object *) = ptr->func_ptr;
                func(ptr);
#endif
                if (map_tilemap_recovery_pending)
                {
                    return;
                }
                processed++;
                if (processed >= obj_hitbox_count_enemy)
                {
                    break;
                }
            }
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
/**
 * @brief Resets standard object slots starting from `start_index` and rebuilds the free list.
 * 
 * @param start_index Index to start resetting from (e.g. 0 for full clear, 1 to keep player).
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
/**
 * @brief Resets player hitbox pool and rebuilds its free list.
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
/**
 * @brief Resets enemy hitbox pool and rebuilds its free list.
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

/**
 * @brief Checks if a spawn position is solid and searches nearby tiles for a valid open floor tile if needed.
 * 
 * @param x Target spawn X coordinate.
 * @param y Target spawn Y coordinate.
 * @param w Object width.
 * @param h Object height.
 * @param out_x Pointer to receive valid X coordinate.
 * @param out_y Pointer to receive valid Y coordinate.
 * @return true if a valid spawn position was found (stored in out_x/out_y), false if fully enclosed.
 */
bool ObjectSystem_FindValidSpawnPosition(int16_t x, int16_t y, int16_t w, int16_t h, int16_t * out_x, int16_t * out_y)
{
    if (!MapSystem_IsPositionSolid(x, y, w, h))
    {
        *out_x = x;
        *out_y = y;
        return true;
    }

    // Search pattern offsets in pixels (1-tile and 2-tile radius search around target)
    static const int8_t offset_x[] = {
        0, 16, -16, 0, 16, -16, 16, -16,
        0, 32, -32, 0, 32, -32, 32, -32, 16, 16, -16, -16, 32, 32, -32, -32
    };
    static const int8_t offset_y[] = {
        -16, 0, 0, 16, -16, -16, 16, 16,
        -32, 0, 0, 32, -32, -32, 32, 32, -32, 32, -32, 32, -16, 16, -16, 16
    };

    uint16_t count = sizeof(offset_x) / sizeof(offset_x[0]);
    for (uint16_t i = 0; i < count; i++)
    {
        int16_t cand_x = x + offset_x[i];
        int16_t cand_y = y + offset_y[i];

        if (!MapSystem_IsPositionSolid(cand_x, cand_y, w, h))
        {
            *out_x = cand_x;
            *out_y = cand_y;
            return true;
        }
    }

    *out_x = x;
    *out_y = y;
    return false;
}

/**
 * @brief Instantiates a new standard game object.
 * 
 * @param id               Object type ID (`OBJID_*`).
 * @param x                Initial X coordinate in pixels.
 * @param y                Initial Y coordinate in pixels.
 * @param local_event_flag Flag associated with map triggers or events.
 * @return Index of the allocated object slot in `obj_general`, or -1 if full.
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

    // perform additional checks for sprite slots

    if ((id != OBJID_PLAYER) && (id != OBJID_BOSS_TEST1))
    {
        if (((id >= OBJID_START_OF_DMA_SPRITES) && (id <= OBJID_END_OF_DMA_SPRITES)) ||
            ((id >= OBJID_START_OF_DMA_LIGHT_SPRITES) && (id <= OBJID_END_OF_DMA_LIGHT_SPRITES)) )
        {
            uint16_t k = SpriteEngine_GetVramSlot16(i);
            
            if (k >= 128)
            {
                // Out of VRAM slots - Restore slot back to free list
                obj_general[i].next_free = obj_first_available;
                obj_first_available = i;
                return -1;
            }

            uint16_t temp_tilenum = const_obj_vram_slot_to_tilenum[k];

            obj_general[i].struct_data.npc_data.tilenum = temp_tilenum;
            obj_general[i].struct_data.npc_data.vram_addr = temp_tilenum << 4; // mul by 16, size of a 8x8 tile - this is in words
        }
    }

    struct game_object * p = &obj_general[i];

    p->id = id;
    
    // Inline ObjectSystem_GetUid()
    uint16_t temp_uid = obj_next_uid++;
    if (obj_next_uid == 0)
    {
        obj_next_uid = 1;
    }
    p->uid = temp_uid;

    p->array_index = i;
    
    p->pos.x.lh.h = x;
    p->pos.x.lh.l = 0;
    p->pos.y.lh.h = y;
    p->pos.y.lh.l = 0;

    p->struct_data.npc_data.ttl = 0; // always reset
    p->struct_data.npc_data.ai_scheduled_token = 0xffffffffl;

    p->struct_data.npc_data.ani.frame = 0;

    if ((id >= OBJID_CONST_END_OF_UNSORTED_SPRITES) && (id != OBJID_HITBOX_INVISIBLE) && (id != OBJID_HITBOX_INVISIBLE_E)) // mini objects don't need these
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

        p->struct_data.npc_data.ani.display = 0; // No need to call AniSystem_GetDynamicFrame(p) as the result is unused
    }
    else
    {
        p->struct_data.npc_data.ani.display = 0x5d; // empty tile in fixed mode
    }

    if ((id >= OBJID_START_OF_INTERACTABLES) && (id <= OBJID_END_OF_INTERACTABLES))
    {
        p->state = STATE_SWITCH_OFF;
        p->tile.x = (uint16_t)x >> 4;
        p->tile.y = (uint16_t)y >> 4;

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
        int16_t valid_x = 0;
        int16_t valid_y = 0;
        if (!ObjectSystem_FindValidSpawnPosition(x, y, p->w, p->h, &valid_x, &valid_y))
        {
            // Position is solid / offscreen and no valid open tile nearby - handle failed spawn drops & defeat counter
            Routines_Enemy_HandleFailedSpawn(p);

            if (((id >= OBJID_START_OF_DMA_SPRITES) && (id <= OBJID_END_OF_DMA_SPRITES)) ||
                ((id >= OBJID_START_OF_DMA_LIGHT_SPRITES) && (id <= OBJID_END_OF_DMA_LIGHT_SPRITES)))
            {
                SpriteEngine_ReleaseVramSlot(i, 1);
            }
            p->id = OBJID_NULL;
            p->next_free = obj_first_available;
            obj_first_available = i;
            p->func_ptr = (void *)&Routines_Dummy;
            return -1;
        }

        x = valid_x;
        y = valid_y;
        p->pos.x.lh.h = x;
        p->pos.y.lh.h = y;

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
        p->w = 16;
        p->h = 16;

        if ((id == OBJID_FIREBALL) || (id == OBJID_HITBOX_INVISIBLE))
        {
            p->hit_type = 0x0001;
        }
        else if ((id == OBJID_HITBOX_INVISIBLE_E) || (id == OBJID_BUBBLE_E) || (id == OBJID_ARROW_E))
        {
            p->hit_type = 0x8001;
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

    p->r = x + p->w;
    p->b = y + p->h;

    return i;
}

/*
    Player hitboxes should call this instead
*/
/**
 * @brief Instantiates a new player hitbox object.
 * 
 * @param id Object type ID.
 * @param x  Initial X coordinate.
 * @param y  Initial Y coordinate.
 * @return Index of allocated slot in `obj_hitbox_player`, or -1 if full.
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

    // Player hitboxes don't need VRAM
    struct game_object * p = &obj_hitbox_player[i];

    p->id = id;
    
    // Inline ObjectSystem_GetUid()
    uint16_t temp_uid = obj_next_uid++;
    if (obj_next_uid == 0)
    {
        obj_next_uid = 1;
    }
    p->uid = temp_uid;

    p->array_index = i;
    
    p->pos.x.lh.h = x;
    p->pos.x.lh.l = 0;
    p->pos.y.lh.h = y;
    p->pos.y.lh.l = 0;

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

    p->r = x + p->w;
    p->b = y + p->h;

    return i;
}

/*
    And use this for enemy hitboxes
*/
/**
 * @brief Instantiates a new enemy hitbox object.
 * 
 * @param id Object type ID.
 * @param x  Initial X coordinate.
 * @param y  Initial Y coordinate.
 * @return Index of allocated slot in `obj_hitbox_enemy`, or -1 if full.
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

        uint16_t temp_tilenum = const_obj_vram_slot_to_tilenum[k];

        obj_hitbox_enemy[i].struct_data.npc_data.tilenum = temp_tilenum;
        obj_hitbox_enemy[i].struct_data.npc_data.vram_addr = temp_tilenum << 4; // mul by 16, size of a 8x8 tile - this is in words
    }
    else
    {
        obj_hitbox_enemy_first_available = obj_hitbox_enemy[i].next_free;
    }

    struct game_object * p = &obj_hitbox_enemy[i];

    p->id = id;
    
    // Inline ObjectSystem_GetUid()
    uint16_t temp_uid = obj_next_uid++;
    if (obj_next_uid == 0)
    {
        obj_next_uid = 1;
    }
    p->uid = temp_uid;

    p->array_index = i;
    
    p->pos.x.lh.h = x;
    p->pos.x.lh.l = 0;
    p->pos.y.lh.h = y;
    p->pos.y.lh.l = 0;

    p->struct_data.npc_data.ttl = 0; // always reset
    
    p->struct_data.npc_data.ani.frame = 0;

    p->pos.z.a = 0;
    p->delta.z.a = 0;

    p->state = STATE_IDLE;
    p->facing = FACING_DOWN;
    p->struct_data.npc_data.ani.display = 0; // No need to call AniSystem_GetDynamicFrame_Stateless(p) as the result is unused
    p->struct_data.npc_data.ani.last_address = 0; // make this invalid

    obj_hitbox_count_enemy++;

    p->hit_type = 0x8001;

    p->w = 16;
    p->h = 16;

    ObjectSystem_SetFunctionPointer(p);

    p->r = x + p->w;
    p->b = y + p->h;

    return i;
}

/**
 * @brief Instantiates NPC objects defined in a map spawn list.
 * 
 * @param list     Pointer to the NPC spawn list.
 * @param offset_x Map offset X.
 * @param offset_y Map offset Y.
 * @return Number of NPCs successfully instantiated.
 */
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
            int16_t temp_rand   = (((int16_t)Math_GetRandom_u16()) % list->random_spread) - (list->random_spread >> 1);
            int16_t temp_rand_y = (((int16_t)Math_GetRandom_u16()) % list->random_spread) - (list->random_spread >> 1);

            temp_x = list->x + temp_rand   + offset_x;
            temp_y = list->y + temp_rand_y + offset_y;
        }
        else
        {
            temp_x = list->x + offset_x;
            temp_y = list->y + offset_y;
        }

        if (temp_x < 0 || temp_x >= map_extent_x ||
            temp_y < 0 || temp_y >= map_extent_y) 
        {
            crashhandler_error_code = CRASHHANDLER_ERROR_NPC_OUT_OF_BOUNDS;
            System_CrashHandler();
        }

        if (ObjectSystem_InstantiateObject(temp_objid, temp_x, temp_y, 0) == -1)
        {
            return 1;
        }

        list++;
    }

    return 0;
}

/**
 * @brief Instantiates enemy spawner objects defined in a map spawner list.
 * 
 * @param list Pointer to the spawner list.
 * @return Number of spawners successfully instantiated.
 */
uint16_t ObjectSystem_List_InstantiateSpawners(const struct obj_list_entry_spawners* list)
{
    uint16_t temp_total_spawns = 0;
    
    while (list->id != OBJID_NULL)
    {
        if (obj_active_count >= OBJ_GENERAL_MAX_COUNT)
        {
            return 1;
        }

        uint16_t temp_objid = list->id;
        int16_t temp_x = list->x;
        int16_t temp_y = list->y;

        if (temp_x < 0 || temp_x >= map_extent_x ||
            temp_y < 0 || temp_y >= map_extent_y)
        {
            crashhandler_error_code = CRASHHANDLER_ERROR_SPAWNER_OUT_OF_BOUNDS;
            System_CrashHandler();
        }

        int16_t index = ObjectSystem_InstantiateObject(temp_objid, temp_x, temp_y, 0);

        if (index == -1)
        {
            return 1;
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
        obj_enemies_target_count = (obj_enemies_max_count * 3) / 10;

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

/**
 * @brief Instantiates interactable objects defined in a map interactables list.
 * 
 * @param list Pointer to the interactables list.
 * @return Number of interactable objects successfully instantiated.
 */
uint16_t ObjectSystem_List_InstantiateInteractables(const struct obj_list_entry_interactable* list)
{
    while (list->id != OBJID_NULL)
    {
        uint16_t temp_objid = list->id;
        int16_t temp_x = list->x;
        int16_t temp_y = list->y;
        uint16_t temp_flag = (uint16_t)((uint32_t)(list->flag));

        bool needs_event_flag = 
            (temp_objid == OBJID_INTERACTABLE_SWITCH_WALL) ||
            (temp_objid == OBJID_INTERACTABLE_SWITCH_FLOOR) ||
            (temp_objid == OBJID_INTERACTABLE_BLOCKER_FLOOR) ||
            (temp_objid == OBJID_INTERACTABLE_BLOCKER_DOOR_NS) ||
            (temp_objid == OBJID_INTERACTABLE_BLOCKER_DOOR_EW);


        if (needs_event_flag && temp_flag >= EVENT_FLAG_LOCAL_MAX)
        {
            crashhandler_error_code = CRASHHANDLER_ERROR_INVALID_EVENT_FLAG;
            System_CrashHandler();
        }
        
        if (temp_x < 0 || temp_x >= map_extent_x ||
            temp_y < 0 || temp_y >= map_extent_y)
        {
            crashhandler_error_code = CRASHHANDLER_ERROR_INTERACTABLE_OUT_OF_BOUNDS;
            System_CrashHandler();
        }

        // Check if the interactable is placed on a valid tile (e.g., not outside the map bounds)
        uint16_t tile_x = (uint16_t)temp_x >> 4;
        uint16_t tile_y = (uint16_t)temp_y >> 4;

        if ((temp_objid == OBJID_INTERACTABLE_BLOCKER_DOOR_NS ||
            temp_objid == OBJID_INTERACTABLE_LEVEL_WARP) &&
            tile_x >= map_extent_tiles_x - 1)
        {
            crashhandler_error_code = CRASHHANDLER_ERROR_INVALID_NS_DOOR_WARP;
            System_CrashHandler();
        }

        if ((temp_objid == OBJID_INTERACTABLE_BLOCKER_DOOR_EW ||
            temp_objid == OBJID_INTERACTABLE_LEVEL_WARP) &&
            tile_y == 0)
        {
            crashhandler_error_code = CRASHHANDLER_ERROR_INVALID_EW_DOOR_WARP;
            System_CrashHandler();
        }

        if (obj_active_count >= OBJ_GENERAL_MAX_COUNT)
        {
            return 1;
        }

        int16_t index = ObjectSystem_InstantiateObject(temp_objid, temp_x, temp_y, temp_flag);
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

/**
 * @brief Enqueues a standard game object for deferred destruction at the end of the frame.
 * 
 * @param i Index of the object in `obj_general`.
 */
void ObjectSystem_DestroyStandardObject(uint16_t i)
{
    if (obj_delete_queue_count >= OBJ_GENERAL_MAX_COUNT)
    {
        return;
    }

    obj_delete_queue[obj_delete_queue_count] = i;

    obj_delete_queue_count++;

    return;
}

// Ditto for player hitboxes
/**
 * @brief Enqueues a player hitbox for deferred destruction.
 * 
 * @param i Index in `obj_hitbox_player`.
 */
void ObjectSystem_DestroyPlayerHitbox(uint16_t i)
{
    if (obj_hitbox_player_delete_queue_count >= OBJ_PLAYERHITBOX_MAX_COUNT)
    {
        return;
    }
    
    obj_hitbox_player_delete_queue[obj_hitbox_player_delete_queue_count] = i;

    obj_hitbox_player_delete_queue_count++;

    return;
}

// Lastly for enemy hitboxes
/**
 * @brief Enqueues an enemy hitbox for deferred destruction.
 * 
 * @param i Index in `obj_hitbox_enemy`.
 */
void ObjectSystem_DestroyEnemyHitbox(uint16_t i)
{
    if (obj_hitbox_enemy_delete_queue_count >= OBJ_ENEMYHITBOX_MAX_COUNT)
    {
        return;
    }
    
    obj_hitbox_enemy_delete_queue[obj_hitbox_enemy_delete_queue_count] = i;

    obj_hitbox_enemy_delete_queue_count++;

    return;
}

/**
 * @brief Processes `obj_delete_queue` and recycles destroyed standard object slots into the free list.
 */
void ObjectSystem_CleanupStandardObjects()
{
    uint16_t actual_deletions = 0;
    for (int i = 0; i < obj_delete_queue_count; i++)
    {
        uint16_t index_of_deletee = obj_delete_queue[i];
        
        if (index_of_deletee >= OBJ_GENERAL_MAX_COUNT)
        {
            continue;
        }

        if (obj_general[index_of_deletee].id == OBJID_NULL)
        {
            continue;
        }

        // TODO: use a function call return value
        if ((obj_general[index_of_deletee].id == OBJID_SLIME) || (obj_general[index_of_deletee].id == OBJID_LIZARDMAN))
        {
            SpriteEngine_ReleaseVramSlot(index_of_deletee, 1);
        }

        obj_general[index_of_deletee].id = OBJID_NULL;

        // Thread back the object to the next free
        obj_general[index_of_deletee].next_free = obj_first_available;
        obj_first_available = index_of_deletee;

        // Fix the object function to dummy
        obj_general[index_of_deletee].func_ptr = (void *)&Routines_Dummy;
        actual_deletions++;
    }
    
    obj_active_count -= actual_deletions;

    obj_delete_queue_count = 0;
    return;
}

/**
 * @brief Processes `obj_hitbox_player_delete_queue` and recycles free slots.
 */
void ObjectSystem_CleanupPlayerHitboxes()
{
    uint16_t actual_deletions = 0;
    for (int i = 0; i < obj_hitbox_player_delete_queue_count; i++)
    {
        uint16_t index_of_deletee = obj_hitbox_player_delete_queue[i];
        
        if (index_of_deletee >= OBJ_PLAYERHITBOX_MAX_COUNT)
        {
            continue;
        }

        if (obj_hitbox_player[index_of_deletee].id == OBJID_NULL)
        {
            continue;
        }

        obj_hitbox_player[index_of_deletee].id = OBJID_NULL;

        // Thread back the object to the next free
        obj_hitbox_player[index_of_deletee].next_free = obj_hitbox_player_first_available;
        obj_hitbox_player_first_available = index_of_deletee;

        // Fix the object function to dummy
        obj_hitbox_player[index_of_deletee].func_ptr = (void *)&Routines_Dummy;
        actual_deletions++;
    }
    
    obj_hitbox_count_player -= actual_deletions;

    obj_hitbox_player_delete_queue_count = 0;
    return;
}

/**
 * @brief Processes `obj_hitbox_enemy_delete_queue` and recycles free slots.
 */
void ObjectSystem_CleanupEnemyHitboxes()
{
    uint16_t actual_deletions = 0;
    for (int i = 0; i < obj_hitbox_enemy_delete_queue_count; i++)
    {
        uint16_t index_of_deletee = obj_hitbox_enemy_delete_queue[i];
        if (index_of_deletee >= OBJ_ENEMYHITBOX_MAX_COUNT)
        {
            continue;
        }
        
        if (obj_hitbox_enemy[index_of_deletee].id == OBJID_NULL)
        {
            continue;
        }

        unsigned int id = obj_hitbox_enemy[index_of_deletee].id;
        if ((id >= OBJID_START_OF_DMA_LIGHT_SPRITES) && (id <= OBJID_END_OF_DMA_LIGHT_SPRITES))
        {
            SpriteEngine_ReleaseVramSlot(OBJ_GENERAL_MAX_COUNT + index_of_deletee, 1);
        }

        obj_hitbox_enemy[index_of_deletee].id = OBJID_NULL;

        // Thread back the object to the next free
        obj_hitbox_enemy[index_of_deletee].next_free = obj_hitbox_enemy_first_available;
        obj_hitbox_enemy_first_available = index_of_deletee;

        // Fix the object function to dummy
        obj_hitbox_enemy[index_of_deletee].func_ptr = (void *)&Routines_Dummy;
        actual_deletions++;
    }
    
    obj_hitbox_count_enemy -= actual_deletions;

    obj_hitbox_enemy_delete_queue_count = 0;
    return;
}

/**
 * @brief Binds execution, drawing, and collision function pointers based on object ID.
 * 
 * @param o Pointer to the target game object.
 */
void ObjectSystem_SetFunctionPointer(struct game_object * o)
{
    switch (o->id)
    {
        case OBJID_SYS_IMPACT:
            o->func_ptr = (void *)&Routines_Fx_Impact;
            break;
        case OBJID_FX_SMOKE:
            o->func_ptr = (void *)&Routines_Fx_Smoke;
            break;
        case OBJID_HITBOX_INVISIBLE:
            o->func_ptr = (void *)&Routines_Player_InvisibleHit;
            break;
        case OBJID_HITBOX_INVISIBLE_E:
            o->func_ptr = (void *)&Routines_Enemy_InvisibleHit;
            break;
        case OBJID_FIREBALL:
            o->func_ptr = (void *)&Routines_Player_Fireball;
            break;
        case OBJID_BUBBLE_E:
            o->func_ptr = (void *)&Routines_Enemy_Slime_Bubble;
            break;
        case OBJID_ARROW_E:
            o->func_ptr = (void *)&Routines_Enemy_Lizardman_Arrow;
            break;
        case OBJID_DROP_MONEY:
            o->func_ptr = (void *)&Routines_Drops_Money;
            break;
        case OBJID_DROP_REC_MEAT:
            o->func_ptr = (void *)&Routines_Drops_Recovery_Meat;
            break;
        case OBJID_SLIME:
            o->func_ptr = (void *)&Routines_Enemy_Slime;
            break;
        case OBJID_LIZARDMAN:
            o->func_ptr = (void *)&Routines_Enemy_Lizardman;
            break;
        case OBJID_PLAYER:
            o->func_ptr = (void *)&Routines_Player;
            break;
        case OBJID_BOSS_TEST1:
            o->func_ptr = (void *)&Routines_Boss_Test;
            break;
        case OBJID_BOSS_TEST1_ATTACK1:
            o->func_ptr = (void *)&Routines_Boss_Test_Attack_Particle;
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
        case OBJID_NULL:
        default:
            // Unimplemented object
            o->func_ptr = (void *)&Routines_Dummy;
            break;
    }

    return;
}

/**
 * @brief Loads baseline stats (HP, ATK, DEF, speed, drop rewards) for enemy objects.
 * 
 * @param o Pointer to the target game object.
 * @return true if enemy stats were successfully populated, false otherwise.
 */
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

const uint16_t const_obj_vram_slot_to_tilenum[128] = {
    // Row 0 (0x00)
    0x0000, 0x0002, 0x0020, 0x0022, 0x0004, 0x0006, 0x0024, 0x0026, 0x0008, 0x000a, 0x0028, 0x002a, 0x000c, 0x000e, 0x002c, 0x002e,
    // Row 1 (0x40)
    0x0040, 0x0042, 0x0060, 0x0062, 0x0044, 0x0046, 0x0064, 0x0066, 0x0048, 0x004a, 0x0068, 0x006a, 0x004c, 0x004e, 0x006c, 0x006e,
    // Row 2 (0x80)
    0x0080, 0x0082, 0x00a0, 0x00a2, 0x0084, 0x0086, 0x00a4, 0x00a6, 0x0088, 0x008a, 0x00a8, 0x00aa, 0x008c, 0x008e, 0x00ac, 0x00ae,
    // Row 3 (0xc0)
    0x00c0, 0x00c2, 0x00e0, 0x00e2, 0x00c4, 0x00c6, 0x00e4, 0x00e6, 0x00c8, 0x00ca, 0x00e8, 0x00ea, 0x00cc, 0x00ce, 0x00ec, 0x00ee,
    // Row 4 (0x100)
    0x0100, 0x0102, 0x0120, 0x0122, 0x0104, 0x0106, 0x0124, 0x0126, 0x0108, 0x010a, 0x0128, 0x012a, 0x010c, 0x010e, 0x012c, 0x012e,
    // Row 5 (0x140)
    0x0140, 0x0142, 0x0160, 0x0162, 0x0144, 0x0146, 0x0164, 0x0166, 0x0148, 0x014a, 0x0168, 0x016a, 0x014c, 0x014e, 0x016c, 0x016e,
    // Row 6 (0x180)
    0x0180, 0x0182, 0x01a0, 0x01a2, 0x0184, 0x0186, 0x01a4, 0x01a6, 0x0188, 0x018a, 0x01a8, 0x01aa, 0x018c, 0x018e, 0x01ac, 0x01ae,
    // Row 7 (0x1c0)
    0x01c0, 0x01c2, 0x01e0, 0x01e2, 0x01c4, 0x01c6, 0x01e4, 0x01e6, 0x01c8, 0x01ca, 0x01e8, 0x01ea, 0x01cc, 0x01ce, 0x01ec, 0x01ee
};
