extern const uint8_t const_ui_vwf_offsets[];

extern uint16_t vwf_shift;

extern uint16_t vwf_col_start;
extern uint16_t vwf_col;

extern uint16_t vwf_row_start;
extern uint16_t vwf_row;

extern uint16_t vwf_tile_id;
extern uint16_t vwf_tile_id_empty;
extern uint8_t * vwf_string_ptr;
extern uint16_t * vwf_tilemap_ptr;
extern uint16_t * vwf_tilemap_ptr_start;
extern uint16_t vwf_tilemap_len;

extern uint8_t * vwf_tiledata_ptr;
extern uint8_t * vwf_tiledata_ptr_start;

extern uint16_t vwf_tiledata_run;
extern uint16_t vwf_tiledata_advance;
extern uint16_t vwf_tiledata_advance_vram;

extern uint16_t vwf_wram_offset;
extern uint16_t vwf_vram_offset;

extern bool vwf_text_rendered;

extern bool vwf_print_ongoing;
extern bool vwf_print_finished;

uint16_t VwfEngine_PrintText(uint8_t * string, uint8_t * dest, uint8_t * tilemap_dest, int col_ext, int row_ext, int id_offset);
void VwfEngine_PrintText_Gradual_Setup(uint8_t * string, uint8_t * dest, uint8_t * tilemap_dest, int col_ext, int row_ext, int id_offset, int tilemap_len);
uint8_t * VwfEngine_PrintText_Gradual(int len);

#if VBCC_ASM == 1
NO_INLINE void VwfEngine_PrintText_Render(__reg("r4/r5") uint8_t * glyph_ptr, __reg("r6/r7")uint8_t * write_ptr, __reg("a")uint16_t mul);
#else
void VwfEngine_PrintText_Render(uint8_t * glyph_ptr, uint8_t * write_ptr, uint16_t mul);
#endif

void VwfEngine_PrintText_StartNewPage();

void VwfEngine_PrintText_ResetTilemap(uint16_t * ptr, int len);
