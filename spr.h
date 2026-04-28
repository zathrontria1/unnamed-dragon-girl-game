void spr_queue_add_ui_wrapper(int16_t x, int16_t y, uint16_t tileattrib);
void spr_queue_add_front_wrapper(struct game_object * o, uint16_t tileattrib);
void spr_queue_add_normal_wrapper(struct game_object * o, uint16_t tileattrib);
void spr_queue_add_back_wrapper(struct game_object * o, uint16_t tileattrib);

#ifdef __VBCC__
    NO_INLINE void spr_queue_add(__reg("r0/r1") struct spr_queue_entry * s, __reg("r2/r3") struct spr_queue_entry * target_queue);
#else
    void spr_queue_add(struct spr_queue_entry * s, struct spr_queue_entry * target_queue);
#endif

void spr_queue_process(void);

#ifdef __VBCC__
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

