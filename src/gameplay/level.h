typedef enum {
    LEVEL_ID_TEST_0 = 0,
    LEVEL_ID_TEST_1,
    LEVEL_ID_TEST_2,
    LEVEL_ID_COUNT,
    LEVEL_ID_INVALID = 0xffff
} level_id_t;

extern const struct level_data * level_data_ptr;
extern const struct level_data * level_data_ptr_prev;
extern const struct level_data * level_data_ptr_next;

extern const struct level_data * const_level_pointer_table[LEVEL_ID_COUNT];

bool LevelSystem_LoadLevel(const struct level_data * level);
void LevelSystem_LoadLevelGraphics(const struct level_data * level);
void LevelSystem_LoadLevelTileset(const struct level_data * level);
void LevelSystem_LoadLevelPalette(const struct level_data * level);
