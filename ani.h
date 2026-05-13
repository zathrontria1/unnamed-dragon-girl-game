// Lookup tables for animations
// With flipping
extern const uint16_t const_ani_lut_basic[56];

uint16_t ani_animate_drop_gravity(struct game_object * o);

uint8_t * ani_getframe_player(struct game_object * o);
uint16_t ani_getframe_fixed_fast(struct game_object * o);

uint8_t * ani_getframe_dynamic(struct game_object * o);

#if VBCC_ASM == 1
NO_INLINE uint8_t * ani_getframe_dynamic_slime(__reg("a/x") struct game_object * o);
#else
uint8_t * ani_getframe_dynamic_slime(struct game_object * o);
#endif

uint8_t * ani_getframe_dynamic_stateless(struct game_object * o);

#if VBCC_ASM == 1
NO_INLINE uint8_t * ani_getframe_dynamic_bubble(struct game_object * o);
#else
uint8_t * ani_getframe_dynamic_bubble(struct game_object * o);
#endif
