extern const int16_t const_hdma_scroll_sine[16][64];

void HdmaEngine_SetupHdma(void);

void HdmaEngine_SetupPaletteHdma(void);
void HdmaEngine_SetupBgScrollHdma(void);
void HdmaEngine_UpdateBgScrollValues();

void HdmaEngine_GeneratePaletteTable(uint16_t * table_ptr, uint16_t pal_start, uint16_t entries, int16_t intensity_change, int16_t height, uint16_t rate_delay, uint16_t blend_mode);

void HdmaEngine_EnableHdma(void);
