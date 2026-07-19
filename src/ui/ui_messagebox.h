extern const uint16_t const_ui_textadvance_tilemapentries[];

void UserInterface_ClearTextBuffer_Subset(uint16_t row, uint16_t col, uint16_t len);

void UserInterface_DrawTextbox(uint16_t row, uint16_t h);
void UserInterface_ClearTextbox(uint16_t row, uint16_t h);
void UserInterface_ClearTextboxText(uint16_t row, uint16_t h);

void UserInterface_CopyTextboxToVram(uint16_t row, uint16_t h);
