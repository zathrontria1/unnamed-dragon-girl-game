uint16_t move(struct game_object * o);

#if VBCC_ASM == 1
    NO_INLINE void move_nocol_fast(__reg("a/x") struct game_object * o);
#else
    inline void move_nocol_fast(struct game_object * o);
#endif

#if VBCC_ASM == 1
    NO_INLINE void move_nocol_veryfast(__reg("a/x") struct game_object * o);
#else
    inline void move_nocol_veryfast(struct game_object * o);
#endif
