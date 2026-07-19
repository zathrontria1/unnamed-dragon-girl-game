#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "obj.h"

#include "hittest.h"

// Function-like macros to reduce overhead of repeated code. These are used to check the hitboxes of player and enemy objects, respectively.

#define CHECK_HITBOX(idx) \
    if (obj_hitbox_player[idx].id != OBJID_NULL) \
    { \
        if ((obj_hitbox_player[idx].r >= ox) && \
            (or >= obj_hitbox_player[idx].pos.x.lh.h) && \
            (ob >= obj_hitbox_player[idx].pos.y.lh.h) && \
            (obj_hitbox_player[idx].b >= oy)) \
        { \
            return &obj_hitbox_player[idx]; \
        } \
    }

#define CHECK_ENEMY_HITBOX(idx) \
    if (obj_hitbox_enemy[idx].id != OBJID_NULL) \
    { \
        if (obj_hitbox_enemy[idx].hit_type == 0x8001) \
        { \
            if ((obj_hitbox_enemy[idx].r >= tx) && \
                (tr >= obj_hitbox_enemy[idx].pos.x.lh.h) && \
                (tb >= obj_hitbox_enemy[idx].pos.y.lh.h) && \
                (obj_hitbox_enemy[idx].b >= ty)) \
            { \
                if (hit == NULL) \
                { \
                    hit = &obj_hitbox_enemy[idx]; \
                } \
                if (obj_hitbox_enemy[idx].id != OBJID_BOSS_TEST1_ATTACK1) \
                { \
                    obj_hitbox_enemy[idx].struct_data.npc_data.ttl = 1; \
                } \
                else \
                { \
                    obj_hitbox_enemy[idx].struct_data.npc_data.ttl = 6; \
                } \
            } \
        } \
    }

// Call from an enemy to hit test
struct game_object * CollisionCheck_EnemyTestPlayer(struct game_object * o)
{
    if (obj_hitbox_count_player == 0)
    {
        return NULL;
    }

    int16_t ox = o->pos.x.lh.h;
    int16_t oy = o->pos.y.lh.h;
    int16_t or = o->r;
    int16_t ob = o->b;

    CHECK_HITBOX(0)
    CHECK_HITBOX(1)
    CHECK_HITBOX(2)
    CHECK_HITBOX(3)
    CHECK_HITBOX(4)
    CHECK_HITBOX(5)
    CHECK_HITBOX(6)
    CHECK_HITBOX(7)
    CHECK_HITBOX(8)
    CHECK_HITBOX(9)
    CHECK_HITBOX(10)
    CHECK_HITBOX(11)
    CHECK_HITBOX(12)
    CHECK_HITBOX(13)
    CHECK_HITBOX(14)
    CHECK_HITBOX(15)

    return NULL;
}

uint16_t CollisionCheck_InteractableTestPlayerAction(struct game_object * o)
{
    if (event_interaction_x == -32728)
    {
        return 0;
    }

    if ((event_interaction_x + 16) < o->pos.x.lh.h)
    {
        return 0;
    }
    if (o->r < event_interaction_x)
    {
        return 0;
    }
    if ((event_interaction_y + 16) < o->pos.y.lh.h)
    {
        return 0;
    }
    if (o->b < event_interaction_y)
    {
        return 0;
    }

    return 1;
}

// Call from player to hit test
struct game_object * CollisionCheck_PlayerTestEnemy(struct game_object * o)
{
    if (obj_hitbox_count_enemy == 0)
    {
        return NULL;
    }

    // shrink the player's hitbox to 2x2, centered within the original width and height
    int16_t tx = o->pos.x.lh.h + (o->w >> 1) - 1;
    int16_t ty = o->pos.y.lh.h + (o->h >> 1) - 1;
    int16_t tr = tx + 2;
    int16_t tb = ty + 2;

    struct game_object * hit = NULL;

    CHECK_ENEMY_HITBOX(0)
    CHECK_ENEMY_HITBOX(1)
    CHECK_ENEMY_HITBOX(2)
    CHECK_ENEMY_HITBOX(3)
    CHECK_ENEMY_HITBOX(4)
    CHECK_ENEMY_HITBOX(5)
    CHECK_ENEMY_HITBOX(6)
    CHECK_ENEMY_HITBOX(7)
    CHECK_ENEMY_HITBOX(8)
    CHECK_ENEMY_HITBOX(9)
    CHECK_ENEMY_HITBOX(10)
    CHECK_ENEMY_HITBOX(11)
    CHECK_ENEMY_HITBOX(12)
    CHECK_ENEMY_HITBOX(13)
    CHECK_ENEMY_HITBOX(14)
    CHECK_ENEMY_HITBOX(15)

    return hit;
}

// NOTE: Hit tests return 0 on hit.

/*
    Axis-aligned bounding box test between two objects

    Although an equivalent is already used inlined in other functions, this needs to be kept for pickupable objects
    as performance will become worse
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
