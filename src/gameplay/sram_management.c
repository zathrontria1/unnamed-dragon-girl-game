#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "obj.h"
#include "level.h"

#include "sram_management.h"

uint8_t sram_available_slots;

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
            Sram_ClearSlot(0);

            // Check if it's possible to write the new string
            // so immediately check the string again

            uint8_t * str_bank0_retest = p;
            for (int k = 0; k < 8; k++)
            {
                if (*str_bank0_retest != const_sram_verify_str[k])
                {
                    temp_no_sram_found = 1;
                    break;
                }

                str_bank0_retest++;
            }

            break;
        }

        str_bank0++;
    }

    if (temp_no_sram_found)
    {
        sram_available_slots = 0;
        return;
    }

    str_bank0 = p + 8;
    *str_bank0 = 0;

    p = (uint8_t *)SRAM_ADDR + 0x00010000 * (SRAM_BANKS - 1);

    // Clear the other slots
    for (int slot = SRAM_BANKS-1; slot > 0; slot--)
    {
        uint8_t * str_bankother = p;
        for (int i = 0; i < 8; i++)
        {
            if (*str_bankother != const_sram_verify_str[i])
            {
                Sram_ClearSlot(slot);
                break;
            }

            str_bankother++;
        }

        str_bankother = p + 8;
        *str_bankother = (uint8_t)slot;

        p = (uint8_t *)((uint32_t)p - 0x00010000);
    }

    // Finally scan the memory area to determine the highest available bank
    uint8_t temp_highestbank = 0;

    p = (uint8_t *)SRAM_ADDR+8;

    for (int i = 0; i < SRAM_BANKS; i++)
    {
        if (*p > temp_highestbank)
        {
            temp_highestbank = *p;
        }

        p = (uint8_t *)((uint32_t)p + 0x00010000);
    }

    sram_available_slots = temp_highestbank + 1;

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

    void * p = (uint8_t *)(SRAM_ADDR + SRAM_DATA_OFFSET + (0x00010000 * slot));

    // Data to be copied:
    // Current level ID
    // Player object blob
    // Global event flags

    // First, map the current level pointer to a Level ID
    uint16_t level_id = LEVEL_ID_INVALID;
    for (uint16_t i = 0; i < LEVEL_ID_COUNT; i++)
    {
        if (const_level_pointer_table[i] == level_data_ptr)
        {
            level_id = i;
            break;
        }
    }

    // Save the level ID as a 16-bit integer
    uint16_t * temp_id = p;
    *temp_id = level_id;
    temp_id++;
    p = (void *) temp_id;

    // Then the object data
    uint8_t * temp_playerdata = p; // BEWARE: not a pointer of pointer! Data must be copied
    uint8_t * temp_livedata = (uint8_t *)&obj_general[obj_player_index];

    for (int i = 0; i < sizeof(struct game_object); i++)
    {
        *temp_playerdata = *temp_livedata;
        temp_playerdata++;
        temp_livedata++;
    }
    
    p = (void *) temp_playerdata;

    // Lastly the event flags
    uint8_t * temp_ef = p;
    
    for (int i = 0; i < EVENT_FLAG_GLOBAL_MAX; i++)
    {
        *temp_ef = event_flags_global[i];
        temp_ef++;
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
    
    // Total size of the data saved to the slot
    for (uint16_t i = 0; i < sizeof(uint16_t) + sizeof(struct game_object) + EVENT_FLAG_GLOBAL_MAX; i++)
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

    // Integrity verified, so what's left is to validate the map ID itself
    void * p = (uint8_t *)(SRAM_ADDR + SRAM_DATA_OFFSET + (0x00010000 * slot));

    // Restore level pointer via ID lookup
    uint16_t * temp_id = p;
    uint16_t loaded_level_id = *temp_id;
    temp_id++;
    p = (void *) temp_id;

    if (loaded_level_id >= LEVEL_ID_COUNT)
    {
        return false; // Invalid or unknown level ID
    }

    level_data_ptr = const_level_pointer_table[loaded_level_id];

    // Restore player object data
    uint8_t * temp_playerdata = p;
    uint8_t * temp_livedata = (uint8_t *)&obj_general[obj_player_index];
    for (int i = 0; i < sizeof(struct game_object); i++)
    {
        *temp_livedata = *temp_playerdata;
        temp_playerdata++;
        temp_livedata++;
    }
    p = (void *) temp_playerdata;

    // Restore event flags
    uint8_t * temp_ef = p;
    for (int i = 0; i < EVENT_FLAG_GLOBAL_MAX; i++)
    {
        event_flags_global[i] = *temp_ef;
        temp_ef++;
    }

    // Re-resolve the player's function pointers based on its ID to handle linker shifts
    struct game_object * player = &obj_general[obj_player_index];
    ObjectSystem_SetFunctionPointer(player);
    player->data_ptr = 0; // Reset runtime pointer to prevent bad references

    return true;
}

const uint8_t const_sram_verify_str[] = "EIEIMUN!"; // Can use any 8 character string that isn't all 0x00 or 0xff. Will occupy 9 bytes in ROM

const struct level_data * const_level_pointer_table[LEVEL_ID_COUNT] = {
    (void *)&data_level_test_0,
    (void *)&data_level_test_1,
    (void *)&data_level_test_2
};
