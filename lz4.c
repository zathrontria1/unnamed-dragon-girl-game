#include <stdint.h>

#include "vars.h"

#include "lz4.h"
#include "dma.h"

uint8_t LZ4_MVNCodeInWRAM[4];

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
                /*for (uint16_t i = 0; i < temp_literal_count; i++)
                {
                    (*ptr_write++) = (*ptr_read++);
                }*/

                LZ4_Internal_Copy(ptr_read, ptr_write, temp_literal_count);
                ptr_write += temp_literal_count;
                ptr_read += temp_literal_count;

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
                    }

                    temp_copy_count += (*ptr_read++);
                }
                temp_copy_count += 4; // hardcoded minimum

                // write out the match string
                LZ4_Internal_Copy(temp_past_ptr_read, ptr_write, temp_copy_count);

                ptr_write += temp_copy_count;

                temp_frame_bytes_written += temp_copy_count;

                temp_frame_bytes_read += ((uint32_t)ptr_read - temp_start_offset);
                temp_block_bytes_read = ((uint32_t)ptr_read - temp_block_start_offset);
            }
        }
    }

    return temp_frame_bytes_written; 
}

#if VBCC_ASM == 1
NO_INLINE void LZ4_Internal_Copy(__reg("r0/r1") uint8_t * src, __reg("r2/r3") uint8_t * dest, __reg("a") uint16_t len)
#else
inline void LZ4_Internal_Copy(uint8_t * src, uint8_t * dest, uint16_t len)
#endif
{
    // r0 contains source
    // r2 contains destination
    // a contains bytes to copy
    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
	    "\tx16\n"
        "\tcmp #0\n"
        "\tbeq LZ4_Internal_Skip\n"
        "\tphy\n"
        "\tphb\n"
        "\ttax\n"
        "\ta8\n"
        "\tsep #$20\n"
        "\tlda #$6B\n" // RTL opcode
        "\tsta >_LZ4_MVNCodeInWRAM+3\n"
        "\tlda #$54\n" // MVN opcode
        "\tsta >_LZ4_MVNCodeInWRAM\n"
        "\tlda r3\n"
        "\tsta >_LZ4_MVNCodeInWRAM+1\n" // write bank byte of source 
        "\tlda r1\n"
        "\tsta >_LZ4_MVNCodeInWRAM+2\n" // ditto for destination
        "\ta16\n"
        "\trep #$20\n"
        "\ttxa\n"
        "\tdec\n"
        "\tldx r0\n"
        "\tldy r2\n"
        "\tjsl >_LZ4_MVNCodeInWRAM;\n"
        "\tplb\n"
        "\tply\n"
        "LZ4_Internal_Skip:\n"
        );
    #else
    // Source and destination bank independent, just can't cross banks
    for (uint16_t i = 0; i < len; i++)
    {
        (*dest++) = (*src++);
    }
    #endif

    return;
}
