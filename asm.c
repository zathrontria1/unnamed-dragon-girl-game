#include "asm.h"

#ifdef __CALYPSI__
    #include <calypsi/intrinsics65816.h>
#endif

// assembly for instructions that can't be directly specified in C
// Always use the compiler defines, not whether inline ASM is enabled
void emitWAI(void) {
#ifdef __VBCC__
    __asm("\twai\n");
#endif
    
#ifdef __CALYPSI__
    __wait_for_interrupt;
#endif
}

void emitSEI(void) {
#ifdef __VBCC__
    __asm("\tsei\n");
#endif

#ifdef __CALYPSI__
    __disable_interrupts;
#endif
}

void emitCLI(void) {
#ifdef __VBCC__
    __asm("\tcli\n");
#endif

#ifdef __CALYPSI__
    __enable_interrupts;
#endif
}
