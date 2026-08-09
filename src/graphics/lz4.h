#ifndef LZ4_H
#define LZ4_H

#include "consts.h"

uint32_t LZ4_ReadU32LE(const uint8_t * ptr);
uint16_t LZ4_ReadU16LE(const uint8_t * ptr);
uint32_t LZ4_UnpackToWRAM(void * src, void * dest);
uint32_t LZ4_UnpackToVRAM(void * src, uint16_t dest);
int32_t LZ4_GetLength(void * src);
#if VBCC_ASM == 1
    NO_INLINE uint16_t LZ4_DecompressBlock(
        __reg("r0/r1") const uint8_t * src,
        __reg("r2/r3") uint8_t * dest,
        __reg("r10") uint16_t block_size,
        __reg("r11") uint16_t hdmaen);
#else
    uint16_t LZ4_DecompressBlock(
        const uint8_t * src,
        uint8_t * dest,
        uint16_t block_size,
        uint16_t hdmaen);
#endif
uint32_t LZ4_DecompressFrame(void * src, void * dest);

#endif

