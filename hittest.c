#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "obj.h"

#include "hittest.h"

// Call from an enemy to hit test
struct game_object * CollisionCheck_EnemyTestPlayer(struct game_object * o)
{
    struct game_object * p = &obj_hitbox_player[0];
    int16_t ox = o->pos.x.lh.h;
    int16_t oy = o->pos.y.lh.h;
    int16_t or = o->r;
    int16_t ob = o->b;
    
    for (int i = 0; i < OBJ_PLAYERHITBOX_MAX_COUNT; i++)
    {
        if (p->id != OBJID_NULL)
        {
            if ((p->r >= ox) &&
                (or >= p->pos.x.lh.h) &&
                (ob >= p->pos.y.lh.h) &&
                (p->b >= oy))
            {
                return p;
            }
        }
        
        p++;
    }

    return NULL;
}

uint16_t CollisionCheck_InteractableTestPlayerAction(struct game_object * o)
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
struct game_object * CollisionCheck_PlayerTestEnemy(struct game_object * o)
{
    // shrink the player's hitbox
    // for this we'll make a copy
    struct game_object temp = *o;
    temp.w = 2;
    temp.h = 2;

    int16_t tx = temp.pos.x.lh.h;
    int16_t ty = temp.pos.y.lh.h;
    int16_t tr = temp.r;
    int16_t tb = temp.b;

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
                if ((p->r >= tx) &&
                    (tr >= p->pos.x.lh.h) &&
                    (tb >= p->pos.y.lh.h) &&
                    (p->b >= ty))
                {
                    if (hit == NULL)
                    {
                        hit = p;
                    }

                    if (hit->id != OBJID_BOSS_TEST1_ATTACK1)
                    {
                        p->struct_data.npc_data.ttl = 1;
                    }
                    else
                    {
                        p->struct_data.npc_data.ttl = 6; // Give 6 frames buffer
                    }
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
uint16_t CollisionCheck_Aabb_BetweenObjects(struct game_object * a, struct game_object * b)
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
uint16_t CollisionCheck_Aabb_Direct_Square(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t s1, int16_t s2)
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
uint16_t CollisionCheck_Aabb_Direct_Rectangle(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t w1, int16_t w2, int16_t h1, int16_t h2)
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
