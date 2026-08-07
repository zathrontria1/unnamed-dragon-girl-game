#include <stdint.h>

#include "snes/console.h"
#include "consts.h"

#if VBCC_ASM == 0

void SoundInterface_UploadData(uint8_t *data_ptr, uint16_t chunk_len)
{
    for (uint16_t i = 0; i < chunk_len; i++)
    {
        REG_APU01 = *data_ptr++;

        uint8_t temp_index_lobyte = (uint8_t)(i);
        REG_APU00 = temp_index_lobyte;

        while (REG_APU00 != temp_index_lobyte)
        {
            ;
        }
    }

    return;
}

void SoundInterface_UploadData_2byte(uint8_t *data_ptr, uint16_t chunk_len)
{
    uint8_t *data_ptr_2 = (uint8_t *)(data_ptr + chunk_len);
    uint8_t temp_internal_counter = 0x00;

    for (uint16_t i = 0; i < chunk_len; i++)
    {
        REG_APU01 = *data_ptr++;
        REG_APU02 = *data_ptr_2++;

        REG_APU00 = temp_internal_counter;

        while (REG_APU00 != temp_internal_counter)
        {
            ;
        }

        temp_internal_counter++;
    }

    return;
}

void SoundInterface_UploadData_2byte_StreamLoopBlock(uint8_t *data_ptr, uint16_t chunk_len)
{
    uint8_t *data_ptr_2 = (uint8_t *)(data_ptr + chunk_len);
    uint8_t temp_internal_counter = 0x00;

    for (uint16_t i = 0; i < chunk_len; i++)
    {
        REG_APU01 = *data_ptr++;

        if (i == 27)
        {
            REG_APU02 = (*data_ptr_2 | 0x03);
            data_ptr_2++;
        }
        else
        {
            REG_APU02 = *data_ptr_2++;
        }

        REG_APU00 = temp_internal_counter;

        while (REG_APU00 != temp_internal_counter)
        {
            ;
        }

        temp_internal_counter++;
    }

    return;
}

#endif
