#ifndef ROUTINES_ENEMY_AI_H
#define ROUTINES_ENEMY_AI_H

#include <stdint.h>
#include <stdbool.h>

#include "defs_structs.h"

bool Routines_Enemy_Ai_CheckLineOfSight(struct game_object * o, int16_t x, int16_t y);
bool Routines_Enemy_Ai_Process(struct game_object * o, uint32_t dist, int16_t x, int16_t y, uint32_t dist_min, bool allow_flipflop, uint16_t attack_delay, uint8_t archetype);
void Routines_Enemy_Ai_Idle(struct game_object * o);
void Routines_Enemy_Ai_PropagateAwareness(struct game_object * source_o, uint32_t radius_sq);

#endif /* ROUTINES_ENEMY_AI_H */
