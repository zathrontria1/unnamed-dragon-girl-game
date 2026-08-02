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

uint16_t LZ4_ReadU16LE(const uint8_t * ptr)
{
#if LZ4_DIRECT_CAST
    return *((const uint16_t *)ptr);
#else
    return (uint16_t)ptr[0] |
           ((uint16_t)ptr[1] << 8);
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
    uint8_t * ptr_write_start = ptr_write;

    uint16_t temp_hdmaen = shadow_hdmaen; // save the HDMA state 

    // Only do prep for DMA if HDMA isn't in use
    if (!temp_hdmaen)
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

    // data block section start
    // if the 4-byte header is not all zeroes, it's a valid block
    uint32_t temp_block_header = LZ4_ReadU32LE(ptr_read);
    while (temp_block_header != 0x00000000)
    {
        uint32_t temp_data_size = temp_block_header & 0x7fffffff;

        ptr_read += 4;

        if ((temp_block_header & 0x80000000) == 0x80000000)
        {
            // block is uncompressed
            if (!temp_hdmaen)
            {
                DmaSystem_CopyToWram_ShortRun(ADDR_LOWORD(ptr_read), ADDR_LOWORD(ptr_write), (uint16_t)temp_data_size);
            }
            else
            {
                System_CopyBlock(ptr_read, ptr_write, (uint16_t)temp_data_size);
            }

            ptr_read += temp_data_size;
            ptr_write += temp_data_size;
        }
        else 
        {
            // block is lz4 compressed
            uint16_t temp_block_bytes_read = 0;
            uint16_t temp_block_size = (uint16_t)temp_data_size;
            uint8_t temp_length_byte = 0;

            if (temp_data_size != 0)
            {
                do
                {
                    uint8_t temp_token = *ptr_read++;
                    uint16_t temp_literal_count = temp_token >> 4;
                    uint16_t temp_copy_count = temp_token & 0x000f;

                    if (temp_literal_count == 15)
                    {
                        // token length max, add more bytes
                        do
                        {
                            temp_length_byte = *ptr_read++;
                            temp_literal_count += temp_length_byte;
                            temp_block_bytes_read += 1;
                        }
                        while (temp_length_byte == 255);
                    }

                    // write out the literals
                    if (temp_literal_count != 0)
                    {
                        if (!temp_hdmaen)
                        {
                            // Only do this if HDMA isn't in use
                            // This should be fine as it's always ROM to RAM

                            // This branch is cheap all things considered, so it's OK to leave it here.
                            DmaSystem_CopyToWram_ShortRun(ADDR_LOWORD(ptr_read), ADDR_LOWORD(ptr_write), temp_literal_count);
                        }
                        else
                        {
                            System_CopyBlock(ptr_read, ptr_write, temp_literal_count);
                        }

                        ptr_write += temp_literal_count;
                        ptr_read += temp_literal_count;
                    }

                    temp_block_bytes_read += temp_literal_count + 1;

                    // The final sequence may contain literals without a match.
                    if (temp_block_bytes_read == temp_block_size)
                    {
                        break;
                    }

                    // Now to start decoding for real
                    uint16_t temp_offset = LZ4_ReadU16LE(ptr_read);

                    if (temp_offset == 0)
                    {
                        // Corrupted block, abort
                        return 0;
                    }

                    uint8_t * temp_past_ptr_read = ptr_write;

                    temp_past_ptr_read -= temp_offset;

                    ptr_read += 2; // get over the offset section
                    temp_block_bytes_read += 2;

                    if (temp_copy_count == 15)
                    {
                        // copy count max, add more bytes
                        do
                        {
                            temp_length_byte = *ptr_read++;
                            temp_copy_count += temp_length_byte;
                            temp_block_bytes_read += 1;
                        }
                        while (temp_length_byte == 255);
                    }

                    temp_copy_count += 4; // hardcoded minimum

                    // write out the match string
                    System_CopyBlock(temp_past_ptr_read, ptr_write, temp_copy_count);

                    ptr_write += temp_copy_count;
                }
                while (temp_block_bytes_read != temp_block_size);
            }
        }

        temp_block_header = LZ4_ReadU32LE(ptr_read);
    }

    return (uint32_t)(ptr_write - ptr_write_start);
}