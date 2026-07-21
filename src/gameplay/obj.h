extern ZP struct game_object * obj_player_pointer;

extern ZP uint16_t obj_first_available;
extern struct game_object obj_general[OBJ_GENERAL_MAX_COUNT];

extern uint16_t obj_delete_queue[OBJ_GENERAL_MAX_COUNT];
extern uint16_t obj_delete_queue_count;

extern int16_t obj_player_index;
extern uint16_t obj_active_count;

extern uint16_t obj_next_uid;

// Enemy data that shouldn't be in the object area
extern uint16_t obj_enemies_defeated;
extern uint16_t obj_enemies_target_count;
extern uint16_t obj_enemies_max_count;

// Player only data that shouldn't be in the object area
extern uint16_t obj_player_attack_interval;
extern uint16_t obj_player_prev_facing;
extern uint8_t * obj_player_prev_sprframe;
extern uint16_t obj_player_active_fireballs;

extern uint16_t obj_player_health_regen_delay;
extern uint16_t obj_player_health_regen_interval;
extern uint16_t obj_player_health_regen_value;
extern uint16_t obj_player_health_regen_limit;

extern uint16_t obj_player_recovery_drop_pity;

extern uint16_t obj_player_upgrades_bought_hp;
extern uint16_t obj_player_upgrades_bought_attack;
extern uint16_t obj_player_upgrades_bought_defense;

extern uint32_t obj_player_upgrades_cost_hp;
extern uint32_t obj_player_upgrades_cost_attack;
extern uint32_t obj_player_upgrades_cost_defense;

// Hitbox data
extern ZP uint16_t obj_hitbox_player_first_available;
extern struct game_object obj_hitbox_player[OBJ_PLAYERHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_player_delete_queue[OBJ_PLAYERHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_player_delete_queue_count;
extern uint16_t obj_hitbox_count_player;

extern ZP uint16_t obj_hitbox_enemy_first_available;
extern struct game_object obj_hitbox_enemy[OBJ_ENEMYHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_enemy_delete_queue[OBJ_ENEMYHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_enemy_delete_queue_count;
extern uint16_t obj_hitbox_count_enemy;

extern const uint16_t const_obj_vram_slot_to_tilenum[128];

void ObjectSystem_ProcessObjects();

void ObjectSystem_ResetStandardObjects(int start_index);
void ObjectSystem_ResetPlayerHitboxes();
void ObjectSystem_ResetEnemyHitboxes();

int16_t ObjectSystem_InstantiateObject(
    uint16_t id,
    int16_t x,
    int16_t y, 
    uint16_t local_event_flag
);

int16_t ObjectSystem_InstantiatePlayerHitbox(
    uint16_t id,
    int16_t x,
    int16_t y
); 

int16_t ObjectSystem_InstantiateEnemyHitbox(
    uint16_t id,
    int16_t x,
    int16_t y
); 

void ObjectSystem_SetFunctionPointer(struct game_object * o);

uint16_t ObjectSystem_List_InstantiateNpcs(const struct obj_list_entry_spawns* list, int16_t offset_x, int16_t offset_y);
uint16_t ObjectSystem_List_InstantiateSpawners(const struct obj_list_entry_spawners* list);
uint16_t ObjectSystem_List_InstantiateInteractables(const struct obj_list_entry_interactable* list);

void ObjectSystem_DestroyStandardObject(uint16_t i);
void ObjectSystem_DestroyPlayerHitbox(uint16_t i);
void ObjectSystem_DestroyEnemyHitbox(uint16_t i);

void ObjectSystem_CleanupStandardObjects(void);
void ObjectSystem_CleanupPlayerHitboxes(void);
void ObjectSystem_CleanupEnemyHitboxes(void);

bool ObjectSystem_GetEnemyData(struct game_object * o);
bool ObjectSystem_FindValidSpawnPosition(int16_t x, int16_t y, int16_t w, int16_t h, int16_t * out_x, int16_t * out_y);
