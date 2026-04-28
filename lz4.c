#include <stdint.h>

#include "lz4.h"
#include "dma.h"

#include "vars.h"

/*
    Unpacks LZ4 compressed data to WRAM area
*/
uint32_t LZ4_UnpackToWRAM(void * src, uint32_t dest)
{
    int32_t temp_length = LZ4_GetLength(src);

    if (temp_length > 0)
    {
        if ((temp_length & 0x01) == 0x01)
        {
            temp_length += 1; // pad to nearest even number
        }

        LZ4_DecompressFrame(src, (void *)LZ4_BUFFER_ADDR);

        dma_copy_to_wram((uint32_t)LZ4_BUFFER_ADDR, dest, temp_length);
    }

    return temp_length;
}

/*
    Unpacks LZ4 compressed data to VRAM area
*/
uint32_t LZ4_UnpackToVRAM(void * src, uint16_t dest)
{
    int32_t temp_length = LZ4_GetLength(src);

    if (temp_length > 0)
    {
        if ((temp_length & 0x01) == 0x01)
        {
            temp_length += 1; // pad to nearest even number
        }
        
        LZ4_DecompressFrame(src, (void *)LZ4_BUFFER_ADDR);

        dma_copy_to_vram((uint32_t)LZ4_BUFFER_ADDR, dest, temp_length);
    }

    return temp_length;
}

int32_t LZ4_GetLength(void * src)
{
    uint8_t * ptr_c = src;
    uint32_t * ptr_dw = src;

    if ((*ptr_dw) != 0x184D2204)
    {
        // Magic ID check failure
        return -1;
    }
    
    ptr_c += 4;

    if (((*ptr_c) & 0x08) != 0x08)
    {
        // Content size is not found
        return -1;
    }

    ptr_c += 2;
    ptr_dw = (uint32_t *)ptr_c;
    return (*ptr_dw);
}

/*
    Decompress an LZ4 frame at address src to destination dest
*/
uint32_t LZ4_DecompressFrame(void * src, void * dest)
{
    uint8_t * ptr_read = src;
    uint8_t * ptr_write = dest;

    if ((*((uint32_t *)ptr_read)) != 0x184D2204)
    {
        // Magic ID check failure
        return 0;
    }
    
    ptr_read += 4;

    if (((*ptr_read) & 0x08) == 0x08)
    {
        // Content size is found
        ptr_read += 11;
    }
    else
    {
        // no content size bytes
        ptr_read += 3;
    }
    
    uint32_t temp_frame_bytes_written = 0;
    uint32_t temp_start_offset = (uint32_t) ptr_read;

    uint32_t temp_frame_bytes_read = 0;

    // data block section start
    // if the 4-byte header is not all zeroes, it's a valid block
    while ((*((uint32_t *)ptr_read)) != 0x00000000)
    {
        uint32_t temp_block_start_offset = (uint32_t) ptr_read;
        uint32_t temp_data_size = ((*((uint32_t *)ptr_read)) & 0x7fffffff);

        if (((*((uint32_t *)ptr_read)) & 0x80000000) == (0x80000000))
        {
            // block is uncompressed
            ptr_read += 4;
            temp_frame_bytes_read += 4;

            for (uint16_t i = 0; i < temp_data_size; i++)
            {
                (*ptr_write++) = (*ptr_read++);
            }

            temp_frame_bytes_written += temp_data_size;

            temp_frame_bytes_read = (uint32_t)(ptr_read - temp_start_offset);
        }
        else 
        {
            // block is lz4 compressed
            ptr_read += 4;

            uint32_t temp_block_bytes_read = 0;

            while (temp_block_bytes_read < temp_data_size)
            {
                uint16_t temp_literal_count = (*ptr_read) >> 4;
                uint16_t temp_copy_count = (*ptr_read++) & 0x000f;

                //ptr_read++;

                if (temp_literal_count == 15)
                {
                    // token length max, add more bytes
                    while ((*ptr_read) == 255)
                    {
                        temp_literal_count += (*ptr_read++);
                        //ptr_read++;
                    }

                    temp_literal_count += (*ptr_read++);
                    //ptr_read++;
                }

                // write out the literals
                for (uint16_t i = 0; i < temp_literal_count; i++)
                {
                    (*ptr_write++) = (*ptr_read++);
                    //ptr_write++;
                    //ptr_read++;
                }

                temp_frame_bytes_written += temp_literal_count;

                // Now to start decoding for real
                uint16_t temp_offset = (uint16_t)(*((uint32_t *)ptr_read));

                if (temp_offset == 0)
                {
                    // Corrupted block, abort
                    return 0;
                }

                uint8_t * temp_past_ptr_read = ptr_write;

                temp_past_ptr_read -= (uint16_t)(*((uint32_t *)ptr_read));

                ptr_read += 2; // get over the offset section

                if (temp_copy_count == 15)
                {
                    // copy count max, add more bytes
                    while ((*ptr_read) == 255)
                    {
                        temp_copy_count += (*ptr_read++);
                        //ptr_read++;
                    }

                    temp_copy_count += (*ptr_read++);
                    //ptr_read++;
                }
                temp_copy_count += 4; // hardcoded minimum

                // write out the match string
                for (uint16_t i = 0; i < temp_copy_count; i++)
                {
                    (*ptr_write++) = (*temp_past_ptr_read++);
                    //ptr_write++;
                    //temp_lit_ptr_read++;
                }

                temp_frame_bytes_written += temp_copy_count;

                temp_frame_bytes_read += ((uint32_t)ptr_read - temp_start_offset);
                temp_block_bytes_read = ((uint32_t)ptr_read - temp_block_start_offset);
            }
        }
    }

    return temp_frame_bytes_written; 
}
