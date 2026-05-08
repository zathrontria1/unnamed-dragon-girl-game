#if VBCC_ASM == 1
    NO_INLINE void interrupt_vblank_sub();
    NO_INLINE void interrupt_vblank_alt();
#else
    void interrupt_vblank_sub();
    void interrupt_vblank_alt();
#endif
