extern const struct level_data * level_data_ptr;
extern const struct level_data * level_data_ptr_prev;
extern const struct level_data * level_data_ptr_next;

bool LevelSystem_LoadLevel(const struct level_data * level);
void LevelSystem_LoadLevelGraphics(const struct level_data * level);
void LevelSystem_LoadLevelTileset(const struct level_data * level);
void LevelSystem_LoadLevelPalette(const struct level_data * level);
