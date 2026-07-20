#include <stdint.h>

uint32_t LZ4_UnpackToWRAM(void * src, void * dest);
uint32_t LZ4_UnpackToVRAM(void * src, uint16_t dest);
int32_t LZ4_GetLength(void * src);
uint32_t LZ4_DecompressFrame(void * src, void * dest);
