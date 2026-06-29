extern NEAR const uint16_t const_lut_dma_split_lookup[6];

extern NEAR struct dma_entry dma_queue[DMA_QUEUE_MAX_ENTRIES];
extern uint16_t dma_queue_count;
extern uint16_t dma_queue_length;

extern bool dma_filler_enable;
extern uint16_t dma_filler_dest;
extern uint16_t dma_filler_length;

void DmaSystem_ClearWram(uint8_t * dest, uint16_t length);

#if VBCC_ASM == 1
    NO_INLINE void DmaSystem_CopyToWram(
    __reg("r0/r1") uint8_t * src, 
    __reg("r2/r3") uint8_t * dest, 
    __reg("a") uint16_t length);
#else
void DmaSystem_CopyToWram(
    uint8_t * src, 
    uint8_t * dest, 
    uint16_t length);
#endif

/*
    Used if multiple small transfer runs. Allows to set DMAP, BBAD, A1B and WMADDH once
*/
void DmaSystem_CopyToWram_ShortPrep(
    uint8_t src_bank, 
    uint8_t dest_bank);

#if VBCC_ASM == 1
    NO_INLINE void DmaSystem_CopyToWram_ShortRun(
    __reg("r0") uint16_t src, 
    __reg("x") uint16_t dest, 
    __reg("a") uint16_t length);
#else
void DmaSystem_CopyToWram_ShortRun(
    uint16_t src, 
    uint16_t dest, 
    uint16_t length);
#endif

void DmaSystem_CopyToVram(
    uint8_t * src, 
    uint16_t dest, 
    uint16_t length);

void DmaSystem_CopyFromVramToWram(
    uint16_t src, 
    uint8_t * dest, 
    uint16_t length);

void DmaSystem_UploadOam();
void DmaSystem_UploadCgram();

#if VBCC_ASM == 1
    NO_INLINE void DmaSystem_UploadCgram_Subset(uint16_t start, uint16_t len);
#else
    void DmaSystem_UploadCgram_Subset(uint16_t start, uint16_t len);
#endif

uint16_t DmaSystem_AddItemToQueue(
    uint8_t * src, 
    uint16_t dest, 
    uint16_t length,
    uint16_t vmain, 
    uint16_t split);

uint16_t DmaSystem_SetClear(uint16_t dest, uint16_t length);

void DmaSystem_ResetQueue();
void DmaSystem_ProcessQueue();

void DmaSystem_UpdateStripTiles();
void DmaSystem_UpdateFrameTiles();
