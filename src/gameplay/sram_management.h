#define SRAM_LAYOUT_VERSION 2

struct level_data;

typedef struct sram_save_data
{
    uint16_t level_id;
    int32_t player_x;
    int32_t player_y;
    int32_t player_z;
    int32_t player_hp;
    uint32_t player_money;
    uint16_t upgrades_hp;
    uint16_t upgrades_attack;
    uint16_t upgrades_defense;
    uint8_t event_flags[EVENT_FLAG_GLOBAL_MAX];
} sram_save_data_t;

typedef enum {
    SRAM_SLOT_EMPTY = 0,
    SRAM_SLOT_VALID,
    SRAM_SLOT_CORRUPT
} sram_slot_status_t;

extern const uint8_t const_sram_verify_str[];
extern uint8_t sram_available_slots;
extern sram_slot_status_t sram_slot_status[SRAM_BANKS];

void Sram_Check(void);
void Sram_ClearSlot(uint16_t slot);
void Sram_SaveToSlot(uint16_t slot);
bool Sram_LoadFromSlot(uint16_t slot);

void Sram_CaptureState(sram_save_data_t * state);
bool Sram_ValidateState(const sram_save_data_t * state);
bool Sram_ReadSlotData(uint16_t slot, sram_save_data_t * state);
sram_slot_status_t Sram_GetSlotStatus(uint16_t slot);

uint16_t Sram_CalculateChecksum(uint16_t slot);


