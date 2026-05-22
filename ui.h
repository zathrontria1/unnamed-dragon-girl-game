extern uint16_t ui_in_bg2;

// UI cache invalidation stuff
extern uint16_t ui_force_update;
extern int32_t ui_cached_hp;
extern int32_t ui_cached_hp_max;
extern uint32_t ui_cached_money;
extern uint16_t ui_cached_enemy_counter;

// TODO: Handle UI windows and texts generically
extern uint16_t ui_window_background[32][32]; // BG1. Call functions to draw a window here.
extern uint16_t ui_window_text[32][32]; // BG3. Call functions to draw text here.

extern uint8_t ui_show_message_string[31]; // 30 characters + null terminator

// Sub-strings
extern uint16_t ui_hp_gauge[28];
extern uint16_t ui_money_counter[11];
extern uint16_t ui_enemy_counter[9];

extern uint16_t ui_level_status[5];

extern uint32_t ui_display_money;

// UI status and timers
extern uint16_t ui_show_message_ttl;
extern uint16_t ui_show_message_cleared;
extern uint16_t ui_show_message_page;
extern uint8_t * ui_show_message_page_ptr_init;
extern uint8_t * ui_show_message_page_ptr;

void UserInterface_Process(void);
void UserInterface_UpdateHealthCounters(void);
void UserInterface_UpdateMoneyCounters(void);
void UserInterface_UpdateEnemyCounters(void);

void UserInterface_PrintText_MultiLine(uint8_t * string_ptr, uint16_t row, uint16_t col);
void UserInterface_PrintSpecialText(uint8_t * string_ptr);
void UserInterface_PrintText(uint8_t * string_ptr, uint16_t row, uint16_t col);
void UserInterface_PrintText_Mode3(uint8_t * string_ptr, uint16_t row, uint16_t col);

void UserInterface_ClearText(uint16_t len, uint16_t row, uint16_t col);

void UserInterface_DrawTextbox(uint16_t row, uint16_t h);

void UserInterface_ClearTextbox(uint16_t row, uint16_t h);
void UserInterface_ClearTextboxText(uint16_t row, uint16_t h);

// Genericized window drawing functions
void UserInterface_ClearWindowBuffer(bool use_clear_tile);
void UserInterface_ClearTextBuffer();
void UserInterface_ClearTextBuffer_Line(uint16_t y);
void UserInterface_DrawWindowBackground(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void UserInterface_DrawWindowText(char * string_ptr, uint16_t x, uint16_t y);

void UserInterface_CopyUiBuffers();
void UserInterface_CopyTextBuffer_Line(uint16_t y);

void UserInterface_CopyTextboxToVram(uint16_t row, uint16_t h);

void UserInterface_DrawEnemyHealthBar(struct game_object * o);

void UserInterface_CopyUiGraphicsToVram(void);
