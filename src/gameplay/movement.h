#ifndef MOVEMENT_H
#define MOVEMENT_H

#include <stdint.h>
#include <stdbool.h>

#define CORNER_NUDGE_THRESHOLD 5

struct game_object;

bool ObjectSystem_Move_TryNudgeCornerX(struct game_object * o, uint16_t q, uint16_t q2, uint16_t temp_x, uint16_t shiftcount);
bool ObjectSystem_Move_TryNudgeCornerY(struct game_object * o, uint16_t q, uint16_t q2, uint16_t shift_temp_y);

uint16_t ObjectSystem_Move(struct game_object * o);

#if VBCC_ASM == 1
    NO_INLINE void ObjectSystem_MoveWithoutCollision(__reg("a/x") struct game_object * o);
#else
    void ObjectSystem_MoveWithoutCollision(struct game_object * o);
#endif

#if VBCC_ASM == 1
    NO_INLINE void ObjectSystem_MoveWithoutCollision_Fast(__reg("a/x") struct game_object * o);
#else
    void ObjectSystem_MoveWithoutCollision_Fast(struct game_object * o);
#endif

#endif /* MOVEMENT_H */
