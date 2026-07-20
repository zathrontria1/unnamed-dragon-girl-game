#include "consts.h"

#ifdef __CALYPSI__
    __attribute__((interrupt(0xffea))) void __irq_vblank(void);
    __attribute__((interrupt(0xffee))) void __irq_ext(void);
    __attribute__((interrupt(0xffe6))) void __irq_brk(void);
    __attribute__((interrupt(0xffe4))) void __irq_cop(void);
    __attribute__((interrupt(0xffe4))) void __irq_cop6502(void);
    __attribute__((interrupt(0xffe4))) void __irq_ext6502(void);
#else
    NEAR INTERRUPT void __irq_vblank(void);
    NEAR INTERRUPT void __irq_ext(void);
    NEAR INTERRUPT void __irq_brk(void);
    NEAR INTERRUPT void __irq_cop(void);
    NEAR INTERRUPT void __irq_cop6502(void);
    NEAR INTERRUPT void __irq_ext6502(void);
#endif
