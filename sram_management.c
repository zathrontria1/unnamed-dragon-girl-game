#include <stdint.h>

#include "vars.h"

#include "sram_management.h"

// Note: the game is configured with 128KB SRAM, divided into 16 banks of 8KB.

/*  Check each SRAM slot for the verification string.
    If any mismatch, wipe the SRAM bank (8KB section)

    Also used to determine if there is any SRAM, and if yes, how many banks are there.
    */
void sram_check()
{
    uint8_t * p = (uint8_t *)SRAM_ADDR;

    uint16_t temp_no_sram_found = 0;

    // First, check the first bank to see if there is SRAM in the first place
    uint8_t * str_bank0 = p;
    for (int i = 0; i < 8; i++)
    {
        if (*str_bank0 != const_sram_verify_str[i])
        {
            sram_clear(0);

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
                sram_clear(slot);
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

void sram_clear(uint16_t slot)
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

void sram_save(uint16_t slot)
{
    if (slot >= SRAM_BANKS)
    {
        return; // if slot count is invalid ignore it
    }

    void * p = (uint8_t *)(SRAM_ADDR + SRAM_DATA_OFFSET + (0x00010000 * slot));

    // Data to be copied:
    // Current level pointer
    // Player object blob
    // Global event flags

    // First the level pointer
    const struct level_data * * temp_ld = p; // BEWARE: pointer of pointer
    
    *temp_ld = level_data_ptr;
    temp_ld++;

    p = (void *) temp_ld;

    // Then the object data
    uint8_t * temp_playerdata = p; // BEWARE: not a pointer of pointer! Data must be copied
    uint8_t * temp_livedata = (uint8_t *)&objects[obj_player_index];

    for (int i = 0; i < sizeof(struct game_object); i++)
    {
        *temp_playerdata = *temp_livedata;
        temp_playerdata++;
        temp_livedata++;
    }

    // below is bugged
    /*struct game_object * temp_playerdata = p; // BEWARE: not a pointer of pointer! Data must be copied

    *temp_playerdata = objects[obj_player_index];
    temp_playerdata++;*/
    
    p = (void *) temp_playerdata;

    // Lastly the event flags
    uint8_t * temp_ef = p;
    
    for (int i = 0; i < EVENT_FLAG_GLOBAL_MAX; i++)
    {
        *temp_ef = event_flags_global[i];
        temp_ef++;
    }

    return;
}