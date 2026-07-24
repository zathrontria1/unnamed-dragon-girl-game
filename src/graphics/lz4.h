#ifndef LZ4_H
#define LZ4_H

#include <stdint.h>
uint32_t LZ4_ReadU32LE(const uint8_t * ptr);
uint32_t LZ4_UnpackToWRAM(void * src, void * dest);
uint32_t LZ4_UnpackToVRAM(void * src, uint16_t dest);
int32_t LZ4_GetLength(void * src);
uint32_t LZ4_DecompressFrame(void * src, void * dest);

#endif

