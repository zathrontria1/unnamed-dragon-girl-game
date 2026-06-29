extern const uint8_t const_sram_verify_str[];
extern uint8_t sram_available_slots;

void Sram_Check(void);
void Sram_ClearSlot(uint16_t slot);
void Sram_SaveToSlot(uint16_t slot);
