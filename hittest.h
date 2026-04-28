struct game_object * hit_test_enemy(struct game_object * o);
struct game_object * hit_test_player(struct game_object * o);
uint16_t hit_test_interaction(struct game_object * o);
#ifdef __VBCC__
    NO_INLINE uint16_t hit_test_blocker(struct tile_xy t);
#else
    uint16_t hit_test_blocker(struct tile_xy t);
#endif
uint16_t hit_test_direct(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t s1, int16_t s2);
uint16_t hit_test(struct game_object * a, struct game_object * b);
uint16_t hit_test_extended(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t w1, int16_t w2, int16_t h1, int16_t h2);
uint16_t hit_test_blocker(struct tile_xy t);
