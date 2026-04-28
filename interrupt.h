#ifdef __CALYPSI__
    __attribute__((interrupt(0xffea))) void __irq_vblank(void);
#else
    NEAR INTERRUPT void __irq_vblank(void);
#endif
