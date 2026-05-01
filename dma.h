void dma_copy_to_wram(
    uint32_t src, 
    uint32_t dest, 
    uint16_t length);

void dma_copy_to_vram(
    uint32_t src, 
    uint16_t dest, 
    uint16_t length);

void dma_clear_vram(void);
void dma_copy_oam(void);
void dma_copy_palette(void);
void dma_copy_palette_subset(uint16_t start, uint16_t len);

inline uint16_t dma_queue_add(
    uint8_t * src, 
    uint16_t dest, 
    uint16_t length,
    uint16_t vmain, 
    uint16_t split);

void dma_queue_process(void);

void dma_copy_bg_water_anim(void);
void dma_copy_bg_64height_anim(void);
