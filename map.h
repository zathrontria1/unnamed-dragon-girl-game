void MapSystem_LoadMap(const uint8_t * map, const uint16_t * lut, const uint8_t * col);
void MapSystem_BuildCollisionTable();

void MapSystem_UpdateCameraPosition(uint16_t suppress_map_gen);
void MapSystem_CheckCrossedTilemapEdge();

void MapSystem_Tilemap_RegenerateTilemap(void);
uint16_t MapSystem_Tilemap_BuildColumn(const uint8_t * p, const uint16_t * lut, int16_t tile_x, int16_t tile_y, uint16_t odd);
void MapSystem_Tilemap_BuildRow(const uint8_t * p, const uint16_t * lut, int16_t tile_x, int16_t tile_y, uint16_t odd);
