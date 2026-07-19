void Routines_Fx_Impact(struct game_object * o);
void Routines_Fx_Smoke(struct game_object * o);

void Routines_Interactables_Switch(struct game_object * o);
void Routines_Interactable_Blocker(struct game_object * o);
void Routines_Interactable_Sign(struct game_object * o);

void Routines_LevelWarp(struct game_object * o);

void Routines_TreasureChest(struct game_object * o);

void Routines_EnemySpawner(struct game_object * o);

void Routines_Drops_Money(struct game_object * o);
void Routines_Drops_Recovery_Meat(struct game_object * o);

void Routines_Dummy(struct game_object * o);

bool Routines_Shared_StatusMaintenance(struct game_object * o);
void Routines_Shared_CheckIfDead(struct game_object * o);

void Routines_Shared_Draw(struct game_object * o, uint8_t * spr_addr, uint16_t pal_shifted, int layer, bool always_flicker, bool is_player);
void Routines_Shared_DrawFixed(struct game_object * o, uint16_t tileattrib, int layer, bool always_flicker);
