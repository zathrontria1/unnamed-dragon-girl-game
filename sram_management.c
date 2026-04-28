#include <stdint.h>

#include "vars.h"

#include "sram_management.h"

// Note: the game is configured with 128KB SRAM, divided into 16 banks of 8KB.

/*  Check each SRAM slot for the verification string.
    If any mismatch, wipe the SRAM bank (8KB section)
    */
void sram_check()
{
    uint8_t * p = (uint8_t *)SRAM_ADDR;

    for (int slot = 0; slot < SRAM_BANKS; slot++)
    {
        uint8_t * str = p;
        for (int i = 0; i < 8; i++)
        {
            if (*str != const_sram_verify_str[i])
            {
                sram_clear(slot);
                break;
            }

            str++;
        }

        p = (uint8_t *)((uint32_t)p + 0x00010000);
    }

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