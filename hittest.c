#include <stdlib.h>
#include <stdint.h>

#include "vars.h"

#include "hittest.h"

// Call from an enemy to hit test
inline struct game_object * hit_test_enemy(struct game_object * o)
{
    int temp_objects_encountered = 0;

    struct game_object * p = &objects[0];

    // There's a bug with the cached list.
    // For now reverted to testing all objects.
    for (int i = 0; temp_objects_encountered < hitbox_count_player_shadow; i++)
    {
        if (p->id != OBJID_NULL)
        {
            if (p->hit_type == 0x0001)
            {
                if (hit_test(o, p) == 0)
                {
                    return p;
                }

                temp_objects_encountered++;
            }
        }

        p++;
    }

    return NULL;
}

inline uint16_t hit_test_interaction(struct game_object * o)
{
    int16_t x1 = o->pos.x.lh.h;
    int16_t y1 = o->pos.y.lh.h;

    if (hit_test_direct(x1, event_interaction_x, y1, event_interaction_y, 16, 16) == 0)
    {
        return 1;
    }

    return 0;
}

// Call from player to hit test
inline struct game_object * hit_test_player(struct game_object * o)
{
    // shrink the player's hitbox
    // for this we'll make a copy
    struct game_object temp = *o;
    temp.w = 2;
    temp.h = 2;

    struct game_object * hit = NULL;
    struct game_object * p = &objects[0];

    int temp_objects_encountered = 0;

    // There's a bug with the cached list.
    // For now reverted to testing all objects.
    for (int i = 0; temp_objects_encountered < hitbox_count_enemy_shadow; i++)
    {
        // Test only objects that participate
        if (p->id != OBJID_NULL)
        {
            if (p->hit_type == 0x8001)
            {
                if (hit_test(&temp, p) == 0)
                {
                    if (hit == NULL)
                    {
                        hit = p;
                    }
                    
                    p->ttl = 1;
                }

                temp_objects_encountered++;
            }
        }

        p++;
    }

    return hit;
}

// Hit tests return 0 on hit.

/*
    Two game objects
*/
inline uint16_t hit_test(struct game_object * a, struct game_object * b)
{
    // a.x < b.x + b.w && b.x < a.x + a.w
    if ((b->pos.x.lh.h + b->w) < a->pos.x.lh.h)
    {
        return 1;
    }
    else if ((a->pos.x.lh.h + a->w) < b->pos.x.lh.h)
    {
        return 1;
    }

    // a.y < b.y + b.h && b.y < a.y + a.h
    if ((a->pos.y.lh.h + a->h) < b->pos.y.lh.h)
    {
        return 1;
    }
    else if ((b->pos.y.lh.h + b->h) < a->pos.y.lh.h)
    {
        return 1;
    }
    return 0;
}

/*
    Directly specified with square size
*/
inline uint16_t hit_test_direct(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t s1, int16_t s2)
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
*/
inline uint16_t hit_test_extended(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t w1, int16_t w2, int16_t h1, int16_t h2)
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

// Call from any moving entity to hit test against dynamic blocked tiles.
// Note: send tile level positions, in the form of 16-bit X followed by 16-bit Y in a single 32-bit variable
// Returns 1 on hit
#if VBCC_ASM == 1
    NO_INLINE uint16_t hit_test_blocker(struct tile_xy t)
#else
    uint16_t hit_test_blocker(struct tile_xy t)
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\tlda _blocker_build_count_shadow\n"
            "\tbeq .no_hits\n"

            "\tasl\n"
            "\tasl\n"
            "\ttax\n"

            "\tlda 4,s\n"
            "\tsta r2\n"
            "\tlda 6,s\n"
            "\tsta r3\n"
        ".check_blocker:\n"
            "\tlda r2\n"
            "\tcmp >_blocker_list,x\n"
            "\tbne .check_next_blocker\n"

            "\tlda r3\n"
            "\tcmp >_blocker_list+2,x\n"
            "\tbne .check_next_blocker\n"

            "\tlda #1\n"
            "\trtl\n"
        ".check_next_blocker:\n"
            "\tdex\n"
            "\tdex\n"
            "\tdex\n"
            "\tdex\n"

            "\tbpl .check_blocker\n"
        ".no_hits:\n"
        );
    #else
        for (int k = 0; k < blocker_build_count_shadow; k++)
        {
            if ((t.x == blocker_list[k].x) && (t.y == blocker_list[k].y))
            {
                return 1;
            }
        }
    #endif

    return 0;
}
