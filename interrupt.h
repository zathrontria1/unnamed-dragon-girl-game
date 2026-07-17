#ifdef __CALYPSI__
    __attribute__((interrupt(0xffea))) void __irq_vblank(void);
#else
    NEAR INTERRUPT void __irq_vblank(void);
#endif

#ifdef __CALYPSI__
    __attribute__((interrupt(0xffee))) void __irq_ext(void);
#else
    NEAR INTERRUPT void __irq_ext(void);
#endif

#ifdef __CALYPSI__
    __attribute__((interrupt(0xffe6))) void __irq_brk(void);
#else
NEAR INTERRUPT void __irq_brk(void);
#endif

#ifdef __CALYPSI__
    __attribute__((interrupt(0xffe4))) void __irq_cop(void);
#else
NEAR INTERRUPT void __irq_cop(void);
#endif

#ifdef __CALYPSI__
    __attribute__((interrupt(0xffe4))) void __irq_cop6502(void);
#else
NEAR INTERRUPT void __irq_cop6502(void);
#endif

#ifdef __CALYPSI__
    __attribute__((interrupt(0xffe4))) void __irq_ext6502(void);
#else
NEAR INTERRUPT void __irq_ext6502(void);
#endif

