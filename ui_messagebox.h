// These functions are currently separated out for organization purposes
// Some of these will also be edited later as they're redundant at times...

extern uint16_t ui_show_message_char_col;
extern uint16_t ui_show_message_char_row;

extern bool ui_show_message_finished;
extern uint16_t ui_show_message_linewidth[4];

void UserInterface_ClearTextBuffer_Subset(uint16_t row, uint16_t col, uint16_t len);
void UserInterface_PrintText_MultiLine(uint8_t * string_ptr, uint16_t row, uint16_t col);
void UserInterface_DrawTextbox(uint16_t row, uint16_t h);
void UserInterface_ClearTextbox(uint16_t row, uint16_t h);
void UserInterface_ClearTextboxText(uint16_t row, uint16_t h);

void UserInterface_PrintText_PerChar();
void UserInterface_PrintText_All();

void UserInterface_CopyTextboxToVram(uint16_t row, uint16_t h);
