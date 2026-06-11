#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "obj.h"

#include "hittest.h"

// Call from an enemy to hit test
#if VBCC_ASM == 1
    NO_INLINE struct game_object * CollisionCheck_EnemyTestPlayer(__reg("r0/r1") struct game_object * o)
#else
    FORCE_INLINE struct game_object * CollisionCheck_EnemyTestPlayer(struct game_object * o)
#endif
{
    #if VBCC_ASM == 1 // place addresses at r0 and r2
        __asm(
        "\ta16\n"
	    "\tx16\n"

        "\tlda #<_obj_hitbox_player\n"
        "\tsta r2\n"
        "\tlda #^_obj_hitbox_player\n"
        "\tsta r3\n"
        
        ".hittest_enemy2player_process_loop:\n"
        "\tlda [r2]\n" // assumes object memory is in bank 7e
        "\tbeq .hittest_enemy2player_increment\n"
        "\tjsl >_CollisionCheck_Aabb_BetweenObjects\n"
        "\tcmp #0\n"
        "\tbne .hittest_enemy2player_increment\n"

        "\tldx r3\n"
        "\tlda r2\n"

        "\trtl\n"

        ".hittest_enemy2player_increment:\n"
        "\tlda r2\n"
        "\tclc\n"
        "\tadc #128\n"
        "\tsta r2\n"
        "\tcmp #<_obj_hitbox_player+2048\n"
        "\tbcc .hittest_enemy2player_process_loop\n"

        ".hittest_enemy2player_break_loop:\n"
    );
    #else
    struct game_object * p = &obj_hitbox_player[0];
    
    for (int i = 0; i < OBJ_PLAYERHITBOX_MAX_COUNT; i++)
    {
        if (p->id != OBJID_NULL)
        {
            if (CollisionCheck_Aabb_BetweenObjects(o, p) == 0)
            {
                return p;
            }
        }
        
        p++;
    }
    #endif

    return NULL;
}

FORCE_INLINE uint16_t CollisionCheck_InteractableTestPlayerAction(struct game_object * o)
{
    int16_t x1 = o->pos.x.lh.h;
    int16_t y1 = o->pos.y.lh.h;

    if (CollisionCheck_Aabb_Direct_Square(x1, event_interaction_x, y1, event_interaction_y, 16, 16) == 0)
    {
        return 1;
    }

    return 0;
}

// Call from player to hit test
FORCE_INLINE struct game_object * CollisionCheck_PlayerTestEnemy(struct game_object * o)
{
    // shrink the player's hitbox
    // for this we'll make a copy
    struct game_object temp = *o;
    temp.w = 2;
    temp.h = 2;

    struct game_object * hit = NULL;
    struct game_object * p = &obj_hitbox_enemy[0];

    // There's a bug with the cached list.
    // For now reverted to testing all objects.
    for (int i = 0; i < OBJ_ENEMYHITBOX_MAX_COUNT; i++)
    {
        // Test only objects that participate
        if (p->id != OBJID_NULL)
        {
            if (p->hit_type == 0x8001)
            {
                if (CollisionCheck_Aabb_BetweenObjects(&temp, p) == 0)
                {
                    if (hit == NULL)
                    {
                        hit = p;
                    }

                    p->struct_data.npc_data.ttl = 1;
                }
            }
        }

        p++;
    }

    return hit;
}

// NOTE: Hit tests return 0 on hit.

/*
    Axis-aligned bounding box test between two objects
*/
#if VBCC_ASM == 1
    NO_INLINE uint16_t CollisionCheck_Aabb_BetweenObjects(__reg("r0/r1") struct game_object * a, __reg("r2/r3") struct game_object * b)
#else
    FORCE_INLINE uint16_t CollisionCheck_Aabb_BetweenObjects(struct game_object * a, struct game_object * b)
#endif
{
    // a.x < b.x + b.w && b.x < a.x + a.w
    if ((b->r) < a->pos.x.lh.h)
    {
        return 1;
    }
    else if ((a->r) < b->pos.x.lh.h)
    {
        return 1;
    }

    // a.y < b.y + b.h && b.y < a.y + a.h
    if ((a->b) < b->pos.y.lh.h)
    {
        return 1;
    }
    else if ((b->b) < a->pos.y.lh.h)
    {
        return 1;
    }
    return 0;
}

/*
    Directly specified with square size

    Slower, but enables not using game objects
*/
FORCE_INLINE uint16_t CollisionCheck_Aabb_Direct_Square(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t s1, int16_t s2)
{
    // a.x < b.x + b.w && b.x < a.x + a.w
    if ((x2 + s2) < x1)
    {
        return 1;
    }
    else if ((x1 + s1) < x2)
    {
        return 1;
    }

    // a.y < b.y + b.h && b.y < a.y + a.h
    if ((y1 + s1) < y2)
    {
        return 1;
    }
    else if ((y2 + s2) < y1)
    {
        return 1;
    }
    return 0;
}

/*
    Directly specified with non-square size

    Slowest (most stack pushes), but allows any size tests
*/
FORCE_INLINE uint16_t CollisionCheck_Aabb_Direct_Rectangle(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t w1, int16_t w2, int16_t h1, int16_t h2)
{
    // a.x < b.x + b.w && b.x < a.x + a.w
    if ((x2 + w2) < x1)
    {
        return 1;
    }
    else if ((x1 + w1) < x2)
    {
        return 1;
    }

    // a.y < b.y + b.h && b.y < a.y + a.h
    if ((y1 + h1) < y2)
    {
        return 1;
    }
    else if ((y2 + h2) < y1)
    {
        return 1;
    }
    return 0;
}
