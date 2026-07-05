extern uint16_t obj_boss_state;
extern bool obj_boss_palette_swap;

extern int obj_boss_phase;
extern int obj_boss_subphase;

extern int obj_boss_timer_movement;
extern bool obj_boss_moving;

extern int obj_boss_timer_attack;

extern uint16_t obj_boss_prev_frame;
extern bool obj_boss_vram_stale;

extern bool obj_boss_hands_show;

extern uint16_t obj_boss_hands_prev_frame;
extern bool obj_boss_hands_vram_stale;

extern int obj_boss_hands_timer_attack;

extern const int16_t const_boss_positions_0[];

void Routines_Boss_Test(struct game_object * o);

bool Routines_Boss_Test_RunPhase(struct game_object * o);

bool Routines_Boss_Test_Attack_Pattern1(struct game_object * o, int32_t x, int32_t y);
void Routines_Boss_Test_Attack_Particle(struct game_object * o); // Subroutine for particle object

bool Routines_Boss_Test_Movement(struct game_object * o, int32_t x, int32_t y);

bool Routines_Boss_Test_TestAgainstHits(struct game_object * o);

void Routines_Boss_Test_ReconstructFrame(struct game_object * o);
void Routines_Boss_Test_DmaFrame(struct game_object * o);
uint8_t * Routines_Boss_Test_GetCompressedFrame(const uint8_t * data, const uint16_t * lookup, uint8_t * buffer, uint16_t frame);
void Routines_Boss_Test_DrawShadow(struct game_object * o);

void Routines_Boss_Test_Hands(struct game_object * o, bool flip);
void Routines_Boss_Test_Hands_DrawShadow(struct game_object * o, bool flip);
void Routines_Boss_Test_Hands_DmaFrame(struct game_object * o);

void Routines_Boss_Test_Draw(struct game_object * o, bool flip);
