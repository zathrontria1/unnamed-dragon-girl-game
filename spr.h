inline void spr_queue_add_ui(int16_t x, int16_t y, uint16_t tileattrib);
inline void spr_queue_add_front(struct game_object * o, uint16_t tileattrib);
inline void spr_queue_add_normal(struct game_object * o, uint16_t tileattrib);
inline void spr_queue_add_back(struct game_object * o, uint16_t tileattrib);

void spr_queue_process(void);

#if VBCC_ASM == 1
    NO_INLINE void spr_draw(__reg("r0/r1") struct spr_queue_entry * s);
#else
    void spr_draw(struct spr_queue_entry * s);
#endif

void spr_init_vram_slot(void);
uint16_t spr_get_vram_slot_16(uint16_t i);
uint16_t spr_get_vram_slot_32(uint16_t i);
void spr_release_vram_slot(uint16_t i, uint16_t slot_count);

void spr_pack_oam(void);
void spr_reset_sprites(void);

