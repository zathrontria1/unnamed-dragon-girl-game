extern uint16_t obj_boss_state;
extern bool obj_boss_palette_swap;

extern int obj_boss_phase;
extern int obj_boss_subphase;

extern int obj_boss_timer;
extern bool obj_boss_moving;

extern const int16_t obj_boss_positions[];

void Routines_Boss_Test(struct game_object * o);

bool Routines_Boss_Test_RunPhase(struct game_object * o);
bool Routines_Boss_Test_Movement(struct game_object * o, int32_t x, int32_t y);

bool Routines_Boss_Test_TestAgainstHits(struct game_object * o);

void Routines_Boss_Test_ReconstructFrame(struct game_object * o);
void Routines_Boss_Test_DmaFrame(struct game_object * o);
uint8_t * Routines_Boss_Test_GetCompressedFrame(const uint8_t * data, const uint16_t * lookup, uint8_t * buffer, uint16_t frame);
void Routines_Boss_Test_DrawShadow(struct game_object * o);
