extern const uint16_t const_ui_vwf_offsets[];

extern uint16_t vwf_shift;
extern uint16_t vwf_col_start;
extern uint16_t vwf_col;
extern uint16_t vwf_row;
extern uint16_t vwf_tile_id;
extern uint8_t * vwf_string_ptr;
extern uint16_t * vwf_tilemap_ptr;
extern uint8_t * vwf_tiledata_ptr;

extern uint16_t vwf_tiledata_run;
extern uint16_t vwf_tiledata_advance;
extern uint16_t vwf_tiledata_advance_vram;

extern bool vwf_text_rendered;

extern bool vwf_print_ongoing;
extern bool vwf_print_finished;

uint16_t VwfEngine_PrintText(uint8_t * string, uint8_t * dest, uint16_t * tilemap_dest, int col_ext, int row_ext, int id_offset);
void VwfEngine_PrintText_Gradual_Setup(uint8_t * string, uint8_t * dest, uint16_t * tilemap_dest, int col_ext, int row_ext, int id_offset);
uint8_t * VwfEngine_PrintText_Gradual(int len);
