void ui_process(void);
void ui_update_health(void);
void ui_update_money(void);
void ui_update_enemy_counters(void);
void ui_print_ml(uint8_t * string_ptr, uint16_t row, uint16_t col);
void ui_print_ml_special(uint8_t * string_ptr);
void ui_print(uint8_t * string_ptr, uint16_t row, uint16_t col);
void ui_print_mode3(uint8_t * string_ptr, uint16_t row, uint16_t col);
void ui_clear(uint16_t len, uint16_t row, uint16_t col);
void ui_draw_textbox(uint16_t row, uint16_t h);
void ui_clear_textbox(uint16_t row, uint16_t h);
void ui_clear_textbox_text(uint16_t row, uint16_t h);
void ui_textbox_dma(uint16_t row, uint16_t h);

void ui_show_enemy_health_bar(struct game_object * o);

void ui_dma_ui_tiles(void);
