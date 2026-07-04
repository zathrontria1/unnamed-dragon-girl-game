struct game_object * CollisionCheck_EnemyTestPlayer(struct game_object * o);

struct game_object * CollisionCheck_PlayerTestEnemy(struct game_object * o);
uint16_t CollisionCheck_InteractableTestPlayerAction(struct game_object * o);

uint16_t CollisionCheck_Aabb_BetweenObjects(struct game_object * a, struct game_object * b);
uint16_t CollisionCheck_Aabb_Direct_Square(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t s1, int16_t s2);
uint16_t CollisionCheck_Aabb_Direct_Rectangle(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t w1, int16_t w2, int16_t h1, int16_t h2);
