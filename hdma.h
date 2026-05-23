extern struct hdma_indirect_table_entry hdma_bgpalette_tables[3];
extern uint16_t hdma_bgpalette_data[448];

extern struct hdma_indirect_table_entry hdma_windowbackground_tables[2][4];
extern uint16_t hdma_windowbackground_data[2][448];
extern uint16_t hdma_windowbackground_select;

extern uint16_t hdma_scroll_data[2][32];
extern uint16_t hdma_scroll_select;
extern ZP uint16_t hdma_scroll_ptr;
extern uint16_t hdma_scroll_sine_index;

extern ZP uint16_t hdma_use_gradient;
extern ZP uint16_t hdma_gradient_ptr;

extern const int16_t const_hdma_scroll_sine[16][64];
extern struct hdma_indirect_table_entry hdma_scroll_tables[2][8];

void HdmaEngine_SetupHdma(void);

void HdmaEngine_SetupPaletteHdma(void);
void HdmaEngine_SetupBgScrollHdma(void);
void HdmaEngine_UpdateBgScrollValues();

void HdmaEngine_GeneratePaletteTable(uint16_t * table_ptr, uint16_t pal_start, uint16_t entries, uint16_t target_color, uint16_t alpha, uint16_t height);

void HdmaEngine_EnableHdma(void);
