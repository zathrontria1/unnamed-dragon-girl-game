extern uint16_t map_column[32]; // one contiguous column
extern uint16_t map_row[2][32]; // two contiguous rows: left and right rows (2x32)

extern const uint8_t * map_current;
extern uint16_t map_extent_x;
extern uint16_t map_extent_y;
extern uint16_t map_extent_tiles_x;
extern uint16_t map_extent_tiles_y;

extern uint16_t map_extent_tiles_x_shiftcount; // converted into amount of shifts. 16 being 4, 32 being 5, 64 being 6, 128 being 7

extern const uint16_t * map_lut;
extern const uint8_t * map_lut_col;

// Collision buffer decompresses here for speed and editability
extern uint8_t map_collision_buf[64*64]; // 4KB // There is no speed benefit from making this 16-bit wide

// Camera/background scroll
ZP extern union pos_bgscroll bg_scroll_x;
ZP extern union pos_bgscroll bg_scroll_y;
extern union pos_bgscroll bg_scroll_x_prev;
extern union pos_bgscroll bg_scroll_y_prev;
ZP extern union pos_bgscroll bg_scroll_y_mod;

extern union pos_bgscroll bg_scroll_x_saved;
extern union pos_bgscroll bg_scroll_y_saved;

extern union pos_bgscroll bg_scroll_x_bounds_min;
extern union pos_bgscroll bg_scroll_y_bounds_min;
extern union pos_bgscroll bg_scroll_x_bounds_max;
extern union pos_bgscroll bg_scroll_y_bounds_max;

extern uint16_t bg_scroll_use_interpolation;
extern uint16_t bg_scroll_x_at_final;
extern uint16_t bg_scroll_y_at_final;
extern uint16_t bg_scroll_suppress_interpolation_state_change;

void MapSystem_LoadMap(const uint8_t * map, const uint16_t * lut, const uint8_t * col);
void MapSystem_BuildCollisionTable();

void MapSystem_UpdateCameraPosition(uint16_t suppress_map_gen);
void MapSystem_CheckCrossedTilemapEdge();

void MapSystem_Tilemap_RegenerateTilemap(void);
uint16_t MapSystem_Tilemap_BuildColumn(const uint8_t * p, const uint16_t * lut, int16_t tile_x, int16_t tile_y, uint16_t odd);
void MapSystem_Tilemap_BuildRow(const uint8_t * p, const uint16_t * lut, int16_t tile_x, int16_t tile_y, uint16_t odd);
