void dma_copy_to_wram(
    uint32_t src, 
    uint32_t dest, 
    uint16_t length);

void dma_copy_to_vram(
    uint32_t src, 
    uint16_t dest, 
    uint16_t length);

void dma_clear_vram(void);
#if VBCC_ASM == 1
    NO_INLINE void dma_copy_oam(void);
#else
    void dma_copy_oam(void);
#endif
#if VBCC_ASM == 1
    NO_INLINE void dma_copy_palette(void);
#else
    void dma_copy_palette(void);
#endif

#if VBCC_ASM == 1
    NO_INLINE void dma_copy_palette_subset(uint16_t start, uint16_t len);
#else
    void dma_copy_palette_subset(uint16_t start, uint16_t len);
#endif

inline uint16_t dma_queue_add(
    uint8_t * src, 
    uint16_t dest, 
    uint16_t length,
    uint16_t vmain, 
    uint16_t split);

#if VBCC_ASM == 1
    NO_INLINE void dma_queue_process(void);
#else
    void dma_queue_process(void);
#endif

#if VBCC_ASM == 1
    NO_INLINE void dma_copy_bg_water_anim(void);
#else
    void dma_copy_bg_water_anim(void);
#endif

#if VBCC_ASM == 1
    NO_INLINE void dma_copy_bg_64height_anim(void);
#else
    void dma_copy_bg_64height_anim(void);
#endif


