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
