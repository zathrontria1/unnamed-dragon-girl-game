#include <stdint.h>

#include "lz4.h"
#include "dma.h"
#include "system.h"

/*
    C fallback
*/
uint32_t LZ4_DecompressBlock(uint8_t **ptr_read, uint8_t **ptr_write, uint16_t block_size, uint16_t hdmaen)
{
    uint8_t *ptr_write_start = *ptr_write;
    uint16_t block_bytes_read = 0;
    uint8_t length_byte = 0;

    if (block_size == 0)
    {
        return 0;
    }

    do
    {
        uint8_t token = *(*ptr_read)++;
        uint16_t literal_count = token >> 4;
        uint16_t copy_count = token & 0x000f;

        if (literal_count == 15)
        {
            do
            {
                length_byte = *(*ptr_read)++;
                literal_count += length_byte;
                block_bytes_read += 1;
            }
            while (length_byte == 255);
        }

        if (literal_count != 0)
        {
            if (hdmaen)
            {
                System_CopyBlock(*ptr_read, *ptr_write, literal_count);
            }
            else
            {
                DmaSystem_CopyToWram_ShortRun(ADDR_LOWORD(*ptr_read), ADDR_LOWORD(*ptr_write), literal_count);
            }

            *ptr_write += literal_count;
            *ptr_read += literal_count;
        }

        block_bytes_read += literal_count + 1;

        if (block_bytes_read == block_size)
        {
            break;
        }

        {
            uint16_t offset = LZ4_ReadU16LE(*ptr_read);
            uint8_t *match_source;

            if (offset == 0)
            {
                return 0;
            }

            match_source = *ptr_write - offset;
            *ptr_read += 2;
            block_bytes_read += 2;

            if (copy_count == 15)
            {
                do
                {
                    length_byte = *(*ptr_read)++;
                    copy_count += length_byte;
                    block_bytes_read += 1;
                }
                while (length_byte == 255);
            }

            copy_count += 4;
            System_CopyBlock(match_source, *ptr_write, copy_count);
            *ptr_write += copy_count;
        }
    }
    while (block_bytes_read != block_size);

    return (uint32_t)(*ptr_write - ptr_write_start);
}