extern uint16_t obj_boss_state;
extern bool obj_boss_palette_swap;

void Routines_Boss_Test(struct game_object * o);

void Routines_Boss_Test_ReconstructFrame(struct game_object * o);
void Routines_Boss_Test_DmaFrame(struct game_object * o);
uint8_t * Routines_Boss_Test_GetCompressedFrame(const uint8_t * data, const uint16_t * lookup, uint8_t * buffer, uint16_t frame);
