#if VBCC_ASM == 1
    NO_INLINE struct game_object * CollisionCheck_EnemyTestPlayer(__reg("r0/r1") struct game_object * o);
#else
    FORCE_INLINE struct game_object * CollisionCheck_EnemyTestPlayer(struct game_object * o);
#endif

FORCE_INLINE struct game_object * CollisionCheck_PlayerTestEnemy(struct game_object * o);
FORCE_INLINE uint16_t CollisionCheck_InteractableTestPlayerAction(struct game_object * o);

#if VBCC_ASM == 1
    NO_INLINE uint16_t CollisionCheck_Aabb_BetweenObjects(__reg("r0/r1") struct game_object * a, __reg("r2/r3") struct game_object * b);
#else
    FORCE_INLINE uint16_t CollisionCheck_Aabb_BetweenObjects(struct game_object * a, struct game_object * b);
#endif
FORCE_INLINE uint16_t CollisionCheck_Aabb_Direct_Square(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t s1, int16_t s2);
FORCE_INLINE uint16_t CollisionCheck_Aabb_Direct_Rectangle(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t w1, int16_t w2, int16_t h1, int16_t h2);
