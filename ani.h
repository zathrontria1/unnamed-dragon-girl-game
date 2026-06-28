
// Lookup tables for animations
// With flipping
extern NEAR const uint16_t const_ani_lut_basic[56];
extern NEAR const uint16_t const_ani_lut_lizardman[56];
extern NEAR const uint16_t const_ani_lut_frame_byteoffsets_16[512];

extern uint16_t buf_player_prev_frame;

uint16_t AniSystem_AnimateDropGravity(struct game_object * o);

uint8_t * AniSystem_GetPlayerFrame(struct game_object * o);

uint16_t AniSystem_GetFixedFrame_Fast(struct game_object * o);

uint8_t * AniSystem_GetDynamicFrame(struct game_object * o);

#if VBCC_ASM == 1
NO_INLINE uint8_t * AniSystem_GetDynamicFrame_Slime(__reg("a/x") struct game_object * o);
#else
uint8_t * AniSystem_GetDynamicFrame_Slime(struct game_object * o);
#endif

#if VBCC_ASM == 1
NO_INLINE uint8_t * AniSystem_GetDynamicFrame_Lizardman(__reg("a/x") struct game_object * o);
#else
uint8_t * AniSystem_GetDynamicFrame_Lizardman(struct game_object * o);
#endif

uint8_t * AniSystem_GetDynamicFrame_Stateless(struct game_object * o);

#if VBCC_ASM == 1
NO_INLINE uint8_t * AniSystem_GetDynamicFrame_Bubble(struct game_object * o);
#else
uint8_t * AniSystem_GetDynamicFrame_Bubble(struct game_object * o);
#endif

uint8_t * AniSystem_GetDynamicFrame_Arrow(struct game_object * o);

uint8_t * AniSystem_GetCompressedFrame(const uint8_t * data, const uint16_t * lookup, uint8_t * buffer, uint16_t frame);
