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
