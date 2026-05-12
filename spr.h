void SpriteEngine_DrawUISprite(int16_t x, int16_t y, uint16_t tileattrib);

#if VBCC_ASM == 1
NO_INLINE void SpriteEngine_AddToFrontLayer(__reg("a/x") struct game_object * o, uint16_t tileattrib);
NO_INLINE void SpriteEngine_AddToSortedLayer(__reg("a/x") struct game_object * o, uint16_t tileattrib);
NO_INLINE void SpriteEngine_AddToBackLayer(__reg("a/x") struct game_object * o, uint16_t tileattrib);
#else
void SpriteEngine_AddToFrontLayer(struct game_object * o, uint16_t tileattrib);
void SpriteEngine_AddToSortedLayer(struct game_object * o, uint16_t tileattrib);
void SpriteEngine_AddToBackLayer(struct game_object * o, uint16_t tileattrib);
#endif

void SpriteEngine_ProcessSpriteLists(void);

#if VBCC_ASM == 1
    NO_INLINE void SpriteEngine_DrawSprite(__reg("r0/r1") struct spr_queue_entry * s);
#else
    void SpriteEngine_DrawSprite(struct spr_queue_entry * s);
#endif

void SpriteEngine_InitVramSlot(void);
uint16_t SpriteEngine_GetVramSlot16(uint16_t i);
uint16_t SpriteEngine_GetVramSlot32(uint16_t i);
void SpriteEngine_ReleaseVramSlot(uint16_t i, uint16_t slot_count);

void SpriteEngine_PackOamHighTable(void);
void SpriteEngine_ResetOam(void);

