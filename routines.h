void routines_fx_impact(struct game_object * o);
void routines_fx_smoke(struct game_object * o);

void routines_interactable_switch(struct game_object * o);
void routines_interactable_blocker(struct game_object * o);
void routines_interactable_sign(struct game_object * o);

void routines_interactable_level_warp(struct game_object * o);

void routines_interactable_treasurechest(struct game_object * o);

void routines_spawner(struct game_object * o);

void routines_drop_money(struct game_object * o);
void routines_drop_rec_meat(struct game_object * o);

void routines_dummy(struct game_object * o);

bool Routines_Shared_StatusMaintenance(struct game_object * o);
void Routines_Shared_CheckIfDead(struct game_object * o);

void Routines_Shared_Draw(struct game_object * o, uint8_t * spr_addr, int pal, int layer, bool always_flicker, bool is_player);
