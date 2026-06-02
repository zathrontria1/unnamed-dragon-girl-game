// These functions are currently separated out for organization purposes
// Some of these will also be edited later as they're redundant at times...

void UserInterface_ClearText(uint16_t len, uint16_t row, uint16_t col);
void UserInterface_PrintText_MultiLine(uint8_t * string_ptr, uint16_t row, uint16_t col);
void UserInterface_DrawTextbox(uint16_t row, uint16_t h);
void UserInterface_ClearTextbox(uint16_t row, uint16_t h);
void UserInterface_ClearTextboxText(uint16_t row, uint16_t h);

void UserInterface_CopyTextboxToVram(uint16_t row, uint16_t h);