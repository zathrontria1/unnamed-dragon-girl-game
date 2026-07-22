#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "obj.h"
#include "level.h"

#include "sram_management.h"

uint8_t sram_available_slots;
sram_slot_status_t sram_slot_status[SRAM_BANKS];

// Note: the game is configured with 32KB SRAM, divided into 4 banks of 8KB.

/**
 * @brief Probes SRAM chip hardware, verifies bank magic strings, and initializes missing save slots.
 */
void Sram_Check()
{
    uint8_t * p = (uint8_t *)SRAM_ADDR;

    uint16_t temp_no_sram_found = 0;

    // First, check the first bank to see if there is SRAM in the first place
    uint8_t * str_bank0 = p;
    for (int i = 0; i < 8; i++)
    {
        if (*str_bank0 != const_sram_verify_str[i])
        {
            uint8_t old_probe_value = p[8];
            uint8_t new_probe_value = (uint8_t)~old_probe_value;
            p[8] = new_probe_value;
            if (p[8] != new_probe_value)
            {
                temp_no_sram_found = 1;
            }
            p[8] = old_probe_value;
            sram_slot_status[0] = SRAM_SLOT_CORRUPT;
            break;
        }

        str_bank0++;
    }

    if (!temp_no_sram_found && sram_slot_status[0] == SRAM_SLOT_EMPTY)
    {
        sram_slot_status[0] = SRAM_SLOT_VALID;
    }

    if (temp_no_sram_found)
    {
        sram_available_slots = 0;
        return;
    }

    p = (uint8_t *)SRAM_ADDR + 0x00010000 * (SRAM_BANKS - 1);

    // Inspect the other slots without modifying them.
    for (int slot = SRAM_BANKS-1; slot > 0; slot--)
    {
        uint8_t * str_bankother = p;
        for (int i = 0; i < 8; i++)
        {
            if (*str_bankother != const_sram_verify_str[i])
            {
                sram_slot_status[slot] = SRAM_SLOT_CORRUPT;
                break;
            }

            str_bankother++;
        }

        if (sram_slot_status[slot] == SRAM_SLOT_EMPTY)
        {
            sram_slot_status[slot] = SRAM_SLOT_VALID;
        }

        p = (uint8_t *)((uint32_t)p - 0x00010000);
    }

    sram_available_slots = 0;
    for (int i = 0; i < SRAM_BANKS; i++)
    {
        if (sram_slot_status[i] != SRAM_SLOT_EMPTY)
        {
            sram_slot_status[i] = Sram_GetSlotStatus(i);
            if (sram_slot_status[i] == SRAM_SLOT_VALID)
            {
                sram_available_slots++;
            }
        }
    }

    return;
}

/**
 * @brief Formats an 8KB SRAM save bank, wiping data and rewriting header magic strings.
 * 
 * @param slot Bank index to format (0 to SRAM_BANKS - 1).
 */
void Sram_ClearSlot(uint16_t slot)
{
    if (slot >= SRAM_BANKS)
    {
        return; // if slot count is invalid ignore it
    }
    uint8_t * p = (uint8_t *)(SRAM_ADDR + (0x00010000 * slot));

    // Write the leading bytes
    int i = 0;
    for (; i < 8; i++)
    {
        *p = const_sram_verify_str[i];
        p++;
    }

    // Wipe the entirety of the bank with 0
    for (; i < SRAM_BANK_SIZE; i++)
    {
        *p++ = 0x00;
    }

    return;
}

/**
 * @brief Serializes current game state (player position/stats, level ID, global flags) into an SRAM slot.
 * 
 * @param slot Target SRAM bank index.
 */
void Sram_SaveToSlot(uint16_t slot)
{
    if (slot >= SRAM_BANKS)
    {
        return; // if slot count is invalid ignore it
    }

    sram_save_data_t state;
    Sram_CaptureState(&state);

    uint8_t * p = (uint8_t *)(SRAM_ADDR + SRAM_DATA_OFFSET + (0x00010000 * slot));
    uint8_t * source = (uint8_t *)&state;
    for (uint16_t i = 0; i < sizeof(sram_save_data_t); i++)
    {
        p[i] = source[i];
    }

    // Compute and save checksum validation details
    uint16_t checksum = Sram_CalculateChecksum(slot);
    
    uint16_t * p_checksum = (uint16_t *)(SRAM_ADDR + 9 + (0x00010000 * slot));
    *p_checksum = checksum;
    
    p_checksum++; // Go forward 2 bytes
    *p_checksum = ~checksum;

    // Write layout version to offset 13 (1 byte)
    uint8_t * p_version = (uint8_t *)(SRAM_ADDR + 13 + (0x00010000 * slot));
    *p_version = SRAM_LAYOUT_VERSION;

    sram_slot_status[slot] = SRAM_SLOT_VALID;

    return;
}

/**
 * @brief Computes a 16-bit additive checksum across all serialized game data bytes in a slot.
 * 
 * @param slot Target SRAM bank index.
 * @return 16-bit unsigned checksum word.
 */
uint16_t Sram_CalculateChecksum(uint16_t slot)
{
    uint8_t * p = (uint8_t *)(SRAM_ADDR + SRAM_DATA_OFFSET + (0x00010000 * slot));
    uint16_t checksum = 0;
    
    for (uint16_t i = 0; i < sizeof(sram_save_data_t); i++)
    {
        checksum += p[i];
    }
    
    return checksum;
}

/**
 * @brief Validates header magic string, layout version, and checksum before restoring game state from SRAM.
 * 
 * @param slot Source SRAM bank index.
 * @return true if game state was successfully restored, false if corrupted or invalid.
 */
bool Sram_LoadFromSlot(uint16_t slot)
{
    if (slot >= SRAM_BANKS)
    {
        return false;
    }

    // Verify verification string (magic number)
    uint8_t * p_verify = (uint8_t *)(SRAM_ADDR + (0x00010000 * slot));
    for (int i = 0; i < 8; i++)
    {
        if (p_verify[i] != const_sram_verify_str[i])
        {
            return false;
        }
    }

    // Verify SRAM layout version (offset 13)
    uint8_t * p_version = (uint8_t *)(SRAM_ADDR + 13 + (0x00010000 * slot));
    if (*p_version != SRAM_LAYOUT_VERSION)
    {
        return false; // Incompatible save format version
    }

    // Read and verify checksum/complement integrity
    uint16_t * p_checksum = (uint16_t *)(SRAM_ADDR + 9 + (0x00010000 * slot));
    uint16_t stored_checksum = *p_checksum;
    
    uint16_t * p_complement = (uint16_t *)(SRAM_ADDR + 11 + (0x00010000 * slot));
    uint16_t stored_complement = *p_complement;

    if (stored_checksum != (uint16_t)(~stored_complement))
    {
        return false; // Header checksum field itself is corrupted
    }

    // Compute and compare data checksum
    uint16_t calculated_checksum = Sram_CalculateChecksum(slot);
    if (calculated_checksum != stored_checksum)
    {
        return false; // Saved game data is corrupted
    }

    sram_save_data_t state;
    uint8_t * source = (uint8_t *)(SRAM_ADDR + SRAM_DATA_OFFSET + (0x00010000 * slot));
    uint8_t * destination = (uint8_t *)&state;
    for (uint16_t i = 0; i < sizeof(sram_save_data_t); i++)
    {
        destination[i] = source[i];
    }

    if (!Sram_ValidateState(&state))
    {
        sram_slot_status[slot] = SRAM_SLOT_CORRUPT;
        return false;
    }

    level_data_ptr = const_level_pointer_table[state.level_id];
    obj_general[obj_player_index].pos.x.a = state.player_x;
    obj_general[obj_player_index].pos.y.a = state.player_y;
    obj_general[obj_player_index].pos.z.a = state.player_z;
    obj_general[obj_player_index].struct_data.npc_data.hp = state.player_hp;
    obj_general[obj_player_index].struct_data.npc_data.money = state.player_money;
    obj_player_upgrades_bought_hp = state.upgrades_hp;
    obj_player_upgrades_bought_attack = state.upgrades_attack;
    obj_player_upgrades_bought_defense = state.upgrades_defense;

    for (uint16_t i = 0; i < EVENT_FLAG_GLOBAL_MAX; i++)
    {
        event_flags_global[i] = state.event_flags[i];
    }

    ObjectSystem_SetFunctionPointer(&obj_general[obj_player_index]);
    obj_general[obj_player_index].data_ptr = 0;
    sram_slot_status[slot] = SRAM_SLOT_VALID;

    return true;
}

void Sram_CaptureState(sram_save_data_t * state)
{
    state->level_id = LEVEL_ID_INVALID;
    for (uint16_t i = 0; i < LEVEL_ID_COUNT; i++)
    {
        if (const_level_pointer_table[i] == level_data_ptr)
        {
            state->level_id = i;
            break;
        }
    }

    state->player_x = obj_general[obj_player_index].pos.x.a;
    state->player_y = obj_general[obj_player_index].pos.y.a;
    state->player_z = obj_general[obj_player_index].pos.z.a;
    state->player_hp = obj_general[obj_player_index].struct_data.npc_data.hp;
    state->player_money = obj_general[obj_player_index].struct_data.npc_data.money;
    state->upgrades_hp = obj_player_upgrades_bought_hp;
    state->upgrades_attack = obj_player_upgrades_bought_attack;
    state->upgrades_defense = obj_player_upgrades_bought_defense;

    for (uint16_t i = 0; i < EVENT_FLAG_GLOBAL_MAX; i++)
    {
        state->event_flags[i] = event_flags_global[i];
    }
}

bool Sram_ValidateState(const sram_save_data_t * state)
{
    if (state->level_id >= LEVEL_ID_COUNT || state->player_hp < 0 || state->player_hp > 0x0000ffffl)
    {
        return false;
    }

    if (state->upgrades_hp > 255 || state->upgrades_attack > 255 || state->upgrades_defense > 255)
    {
        return false;
    }

    return true;
}

bool Sram_ReadSlotData(uint16_t slot, sram_save_data_t * state)
{
    if (slot >= SRAM_BANKS || sram_slot_status[slot] != SRAM_SLOT_VALID)
    {
        return false;
    }

    uint8_t * source = (uint8_t *)(SRAM_ADDR + SRAM_DATA_OFFSET + (0x00010000 * slot));
    uint8_t * destination = (uint8_t *)state;
    for (uint16_t i = 0; i < sizeof(sram_save_data_t); i++)
    {
        destination[i] = source[i];
    }

    return Sram_ValidateState(state);
}

sram_slot_status_t Sram_GetSlotStatus(uint16_t slot)
{
    sram_save_data_t state;

    if (slot >= SRAM_BANKS)
    {
        return SRAM_SLOT_CORRUPT;
    }

    if (sram_slot_status[slot] == SRAM_SLOT_EMPTY)
    {
        return SRAM_SLOT_EMPTY;
    }

    return Sram_ReadSlotData(slot, &state) ? SRAM_SLOT_VALID : SRAM_SLOT_CORRUPT;
}

const uint8_t const_sram_verify_str[] = "EIEIMUN!"; // Can use any 8 character string that isn't all 0x00 or 0xff. Will occupy 9 bytes in ROM

const struct level_data * const_level_pointer_table[LEVEL_ID_COUNT] = {
    (void *)&data_level_test_0,
    (void *)&data_level_test_1,
    (void *)&data_level_test_2
};
