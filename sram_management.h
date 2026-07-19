#define SRAM_LAYOUT_VERSION 1

struct level_data;

typedef enum {
    LEVEL_ID_TEST_0 = 0,
    LEVEL_ID_TEST_1,
    LEVEL_ID_TEST_2,
    LEVEL_ID_COUNT,
    LEVEL_ID_INVALID = 0xffff
} level_id_t;

extern const struct level_data * const_level_pointer_table[LEVEL_ID_COUNT];

extern const uint8_t const_sram_verify_str[];
extern uint8_t sram_available_slots;

void Sram_Check(void);
void Sram_ClearSlot(uint16_t slot);
void Sram_SaveToSlot(uint16_t slot);
bool Sram_LoadFromSlot(uint16_t slot);

uint16_t Sram_CalculateChecksum(uint16_t slot);


