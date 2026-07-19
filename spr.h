extern uint16_t spr_sprite_count; // Rendered sprites this frame
extern uint16_t spr_sprite_count_prev; // previous

extern uint16_t spr_vram_slots[128]; // VRAM slots in sprite page

extern uint16_t spr_front_count; // Rendered non-UI unsorted front-forced sprites this frame
NEAR extern struct spr_queue_entry spr_queue_front[SPR_COUNT_MAX_FRONT];
extern uint16_t spr_back_count; // Rendered non-UI unsorted back-forced sprites this frame (e.g. shadows)
NEAR extern struct spr_queue_entry spr_queue_back[SPR_COUNT_MAX_BACK];

extern uint16_t spr_normal_count;
NEAR extern uint8_t spr_depth_count[257]; // Count of sprites on each depth line
NEAR extern struct spr_queue_entry spr_queue_normal[SPR_COUNT_MAX_SORTED]; // depth sorted sprite entries

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

void SpriteEngine_ProcessSpriteLists();
void SpriteEngine_ProcessSpriteLists_WriteFrontSprites();
void SpriteEngine_ProcessSpriteLists_ClearDepthBuffer();
void SpriteEngine_ProcessSpriteLists_TallySprites();
void SpriteEngine_ProcessSpriteLists_CalculateOffsets();
void SpriteEngine_ProcessSpriteLists_WriteSortedSprites();
void SpriteEngine_ProcessSpriteLists_WriteBackSprites();

#if VBCC_ASM == 1
    NO_INLINE void SpriteEngine_DrawSprite(__reg("r0/r1") struct spr_queue_entry * s);
#else
    void SpriteEngine_DrawSprite(struct spr_queue_entry * s);
#endif

void SpriteEngine_InitVramSlot(void);
uint16_t SpriteEngine_GetVramSlot16(uint16_t i);
uint16_t SpriteEngine_GetVramSlot32(uint16_t i);
void SpriteEngine_ReleaseVramSlot(uint16_t i, uint16_t slot_count);

void SpriteEngine_GetVramForBoss();
void SpriteEngine_ReleaseVramForBoss();

void SpriteEngine_PackOamHighTable();
void SpriteEngine_ResetOam();
