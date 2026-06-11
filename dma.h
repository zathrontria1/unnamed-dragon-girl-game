extern const uint16_t const_lut_dma_split_lookup[6];

extern NEAR struct dma_entry dma_queue[DMA_QUEUE_MAX_ENTRIES];
extern uint16_t dma_queue_count;
extern uint16_t dma_queue_length;

extern bool dma_filler_enable;
extern uint16_t dma_filler_dest;
extern uint16_t dma_filler_length;

void DmaSystem_CopyToWram(
    uint32_t src, 
    uint32_t dest, 
    uint16_t length);

void DmaSystem_CopyToVram(
    uint32_t src, 
    uint16_t dest, 
    uint16_t length);

void DmaSystem_CopyFromVramToWram(
    uint16_t src, 
    uint32_t dest, 
    uint16_t length);

#if VBCC_ASM == 1
    NO_INLINE void DmaSystem_UploadOam(void);
#else
    void DmaSystem_UploadOam(void);
#endif
#if VBCC_ASM == 1
    NO_INLINE void DmaSystem_UploadCgram(void);
#else
    void DmaSystem_UploadCgram(void);
#endif

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

#if VBCC_ASM == 1
    NO_INLINE void DmaSystem_ProcessQueue(void);
#else
    void DmaSystem_ProcessQueue(void);
#endif

#if VBCC_ASM == 1
    NO_INLINE void DmaSystem_UpdateStripTiles(void);
#else
    void DmaSystem_UpdateStripTiles(void);
#endif

#if VBCC_ASM == 1
    NO_INLINE void DmaSystem_UpdateFrameTiles(void);
#else
    void DmaSystem_UpdateFrameTiles(void);
#endif
