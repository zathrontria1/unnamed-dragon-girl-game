inline struct game_object * hit_test_enemy(struct game_object * o);
inline struct game_object * hit_test_player(struct game_object * o);
inline uint16_t hit_test_interaction(struct game_object * o);

#if VBCC_ASM == 1
    NO_INLINE uint16_t hit_test_blocker(struct tile_xy t);
#else
    inline uint16_t hit_test_blocker(struct tile_xy t);
#endif

inline uint16_t hit_test(struct game_object * a, struct game_object * b);
inline uint16_t hit_test_direct(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t s1, int16_t s2);
inline uint16_t hit_test_extended(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t w1, int16_t w2, int16_t h1, int16_t h2);
