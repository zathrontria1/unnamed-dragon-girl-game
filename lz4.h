uint32_t LZ4_UnpackToWRAM(void * src, uint32_t dest);
uint32_t LZ4_UnpackToVRAM(void * src, uint16_t dest);
int32_t LZ4_GetLength(void * src);
uint32_t LZ4_DecompressFrame(void * src, void * dest);

#if VBCC_ASM == 1
NO_INLINE void LZ4_Internal_Copy(__reg("r0/r1") uint8_t * src, __reg("r2/r3") uint8_t * dest, __reg("a") uint16_t len);
#else
inline void LZ4_Internal_Copy(uint8_t * src, uint8_t * dest, uint16_t len);
#endif
