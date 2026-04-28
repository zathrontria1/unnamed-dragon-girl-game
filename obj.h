void obj_run(void);

uint16_t obj_find_free_slot(void);
void obj_reset(void);

int16_t obj_instantiate(
    uint16_t id,
    int16_t x,
    int16_t y, 
    uint16_t local_event_flag
);
uint16_t obj_instantiate_npcs(const struct obj_list_entry_spawns* list, int16_t offset_x, int16_t offset_y);
uint16_t obj_instantiate_spawners(const struct obj_list_entry_spawners* list);
uint16_t obj_instantiate_interactables(const struct obj_list_entry_interactable* list);
uint16_t obj_get_uid(void);

void obj_destroy(uint16_t i);

void obj_cleanup(void);

uint16_t move(struct game_object * o);
void move_nocol_fast(struct game_object * o);

uint32_t ai_distance_squared(int16_t abs_x, int16_t abs_y);
uint16_t ai_run(struct game_object * o, uint32_t dist, int16_t x, int16_t y);
void ai_idle(struct game_object * o);
uint16_t ai_get_facing(struct game_object * o);
