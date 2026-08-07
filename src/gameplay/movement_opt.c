#include <stdint.h>
#include <stdbool.h>

#include "obj.h"
#include "movement.h"

#if VBCC_ASM == 0
void ObjectSystem_MoveWithoutCollision(struct game_object * o)
{
    o->pos.x.a += o->delta.x.a;
    o->pos.y.a += o->delta.y.a;

    o->r = o->pos.x.lh.h + o->w;
    o->b = o->pos.y.lh.h + o->h;

    return;
}

void ObjectSystem_MoveWithoutCollision_Fast(struct game_object * o)
{
    o->pos.x.a += o->delta.x.a;
    o->pos.y.a += o->delta.y.a;

    return;
}
#endif
