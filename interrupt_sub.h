#if VBCC_ASM == 1
    NO_INLINE void interrupt_vblank_sub();
#else
    void interrupt_vblank_sub();
#endif
