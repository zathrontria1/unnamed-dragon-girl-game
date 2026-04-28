void map_load(const uint8_t * map, const uint16_t * lut, const uint8_t * col);
void map_regenerate(void);
void map_camera_adjust(uint16_t suppress_map_gen);
void map_check_tilemap_crossing();
uint16_t map_tilemap_build_col(const uint8_t * p, const uint16_t * lut, int16_t tile_x, int16_t tile_y, uint16_t odd);
void map_tilemap_build_row(const uint8_t * p, const uint16_t * lut, int16_t tile_x, int16_t tile_y, uint16_t odd);
