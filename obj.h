
ZP extern struct game_object * obj_player_pointer;

ZP extern uint16_t obj_first_available;
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
ZP extern uint16_t obj_hitbox_player_first_available;
extern struct game_object obj_hitbox_player[OBJ_PLAYERHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_player_delete_queue[OBJ_PLAYERHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_player_delete_queue_count;
extern uint16_t obj_hitbox_count_player;

ZP extern uint16_t obj_hitbox_enemy_first_available;
extern struct game_object obj_hitbox_enemy[OBJ_ENEMYHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_enemy_delete_queue[OBJ_ENEMYHITBOX_MAX_COUNT];
extern uint16_t obj_hitbox_enemy_delete_queue_count;
extern uint16_t obj_hitbox_count_enemy;

void obj_run(void);

void obj_reset(int start_index);
void obj_reset_hitbox_player();
void obj_reset_hitbox_enemy();

int16_t obj_instantiate(
    uint16_t id,
    int16_t x,
    int16_t y, 
    uint16_t local_event_flag
);

int16_t obj_instantiate_hitbox_player(
    uint16_t id,
    int16_t x,
    int16_t y
); 

int16_t obj_instantiate_hitbox_enemy(
    uint16_t id,
    int16_t x,
    int16_t y
); 

void obj_set_function_pointer(struct game_object * o);

uint16_t obj_instantiate_npcs(const struct obj_list_entry_spawns* list, int16_t offset_x, int16_t offset_y);
uint16_t obj_instantiate_spawners(const struct obj_list_entry_spawners* list);
uint16_t obj_instantiate_interactables(const struct obj_list_entry_interactable* list);
FORCE_INLINE uint16_t obj_get_uid(void);

FORCE_INLINE void obj_destroy(uint16_t i);
FORCE_INLINE void obj_destroy_hitbox_player(uint16_t i);
FORCE_INLINE void obj_destroy_hitbox_enemy(uint16_t i);

void obj_cleanup(void);
void obj_cleanup_hitbox_player(void);
void obj_cleanup_hitbox_enemy(void);

bool obj_get_enemy_data(struct game_object * o);
