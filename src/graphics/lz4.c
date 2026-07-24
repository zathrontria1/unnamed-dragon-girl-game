#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "lz4.h"
#include "dma.h"
#include "system.h"

#ifndef LZ4_DIRECT_CAST
// If this is undefined, the default is to use the direct cast method to read 32-bit values from memory.
// Build scripts by default define this to 1, but if you are having issues with data alignment, you can undefine it to use the byte-by-byte method instead.
#define LZ4_DIRECT_CAST 1 
#endif

/*
    This exists because data alignment may become an issue with certain compilers;

    If possible, use the direct cast method to read the 32-bit value, as it is faster than the byte-by-byte method.

    Only use the byte-by-byte method if you are having issues with data alignment, or if you are using a compiler that does not support the direct cast method.
*/
uint32_t LZ4_ReadU32LE(const uint8_t * ptr)
{
#if LZ4_DIRECT_CAST
    return *((const uint32_t *)ptr);
#else
    return (uint32_t)ptr[0] |
           ((uint32_t)ptr[1] << 8) |
           ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[3] << 24);
#endif
}

/**
 * @brief Unpacks LZ4-compressed data to a WRAM area.
 * 
 * @param src  Pointer to the source LZ4 compressed frame in ROM.
 * @param dest Pointer to the destination buffer in WRAM.
 * @return The length of the decompressed data in bytes, or 0 if the stream is invalid.
 */
uint32_t LZ4_UnpackToWRAM(void * src, void * dest)
{
    int32_t temp_length = LZ4_GetLength(src);

    if (temp_length > 0)
    {
        if ((temp_length & 0x01) == 0x01)
        {
            temp_length += 1; // pad to nearest even number
        }

        LZ4_DecompressFrame(src, dest);
    }

    return temp_length;
}

/**
 * @brief Unpacks LZ4-compressed data to a VRAM area.
 * 
 * Decompresses the frame into a temporary buffer in WRAM, then performs a DMA 
 * copy operation to VRAM.
 * 
 * @param src  Pointer to the source LZ4 compressed frame in ROM.
 * @param dest The word address in VRAM.
 * @return The length of the decompressed data in bytes, or 0 if the stream is invalid.
 */
uint32_t LZ4_UnpackToVRAM(void * src, uint16_t dest)
{
    int32_t temp_length = LZ4_GetLength(src);

    if (temp_length > 0)
    {
        if (temp_length & 0x01)
        {
            temp_length += 1; // pad to nearest even number
        }
        
        LZ4_DecompressFrame(src, (void *)LZ4_BUFFER_ADDR);

        DmaSystem_CopyToVram((uint8_t *)LZ4_BUFFER_ADDR, dest, temp_length);
    }

    return temp_length;
}

/**
 * @brief Extracts the decompressed content size of an LZ4 frame.
 * 
 * Validates the LZ4 magic number (0x184D2204) and parses the headers to find the 
 * decompressed size field.
 * 
 * @param src Pointer to the LZ4 compressed frame.
 * @return The decompressed size of the frame in bytes, or -1 if the frame is invalid/header check fails.
 */
int32_t LZ4_GetLength(void * src)
{
    uint8_t * ptr_c = src;

    if (LZ4_ReadU32LE(ptr_c) != 0x184D2204)
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
    return LZ4_ReadU32LE(ptr_c);
}

/**
 * @brief Decompresses an LZ4 frame into a destination buffer.
 * 
 * @param src  Pointer to the source LZ4 compressed frame.
 * @param dest Pointer to the destination buffer.
 * @return The length of the decompressed data in bytes, or 0 if the stream is invalid.
 */
uint32_t LZ4_DecompressFrame(void * src, void * dest)
{
    uint8_t * ptr_read = src;
    uint8_t * ptr_write = dest;

    // Only do prep for DMA if HDMA isn't in use
    if (!shadow_hdmaen)
    {
        DmaSystem_CopyToWram_ShortPrep(((uint32_t)ptr_read) >> 16, ((uint32_t)ptr_write) >> 16);
    }

    if (LZ4_ReadU32LE(ptr_read) != 0x184D2204)
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
    while (LZ4_ReadU32LE(ptr_read) != 0x00000000)
    {
        uint32_t temp_block_start_offset = (uint32_t) ptr_read;
        uint32_t temp_data_size = (LZ4_ReadU32LE(ptr_read) & 0x7fffffff);

        if ((LZ4_ReadU32LE(ptr_read) & 0x80000000) == (0x80000000))
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

                if (temp_literal_count == 15)
                {
                    // token length max, add more bytes
                    while ((*ptr_read) == 255)
                    {
                        temp_literal_count += (*ptr_read++);
                    }

                    temp_literal_count += (*ptr_read++);
                }

                // write out the literals
                if (temp_literal_count != 0)
                {
                    if (!shadow_hdmaen)
                    {
                        // Only do this if HDMA isn't in use
                        // This should be fine as it's always ROM to RAM

                        // This branch is cheap all things considered, so it's OK to leave it here.
                        DmaSystem_CopyToWram_ShortRun((uint16_t)((uint32_t)ptr_read), (uint16_t)((uint32_t)ptr_write), temp_literal_count);
                    }
                    else
                    {
                        System_CopyBlock(ptr_read, ptr_write, temp_literal_count);
                    }
                    
                    ptr_write += temp_literal_count;
                    ptr_read += temp_literal_count;

                    temp_frame_bytes_written += temp_literal_count;
                }

                // Now to start decoding for real
                uint16_t temp_offset = (uint16_t)LZ4_ReadU32LE(ptr_read);

                if (temp_offset == 0)
                {
                    // Corrupted block, abort
                    return 0;
                }

                uint8_t * temp_past_ptr_read = ptr_write;

                temp_past_ptr_read -= (uint16_t)LZ4_ReadU32LE(ptr_read);

                ptr_read += 2; // get over the offset section

                if (temp_copy_count == 15)
                {
                    // copy count max, add more bytes
                    while ((*ptr_read) == 255)
                    {
                        temp_copy_count += (*ptr_read++);
                    }

                    temp_copy_count += (*ptr_read++);
                }
                temp_copy_count += 4; // hardcoded minimum

                // write out the match string
                System_CopyBlock(temp_past_ptr_read, ptr_write, temp_copy_count);

                ptr_write += temp_copy_count;

                temp_frame_bytes_written += temp_copy_count;

                temp_frame_bytes_read += ((uint32_t)ptr_read - temp_start_offset);
                temp_block_bytes_read = ((uint32_t)ptr_read - temp_block_start_offset);
            }
        }
    }

    return temp_frame_bytes_written; 
}