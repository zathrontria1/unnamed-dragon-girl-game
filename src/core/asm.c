#include "asm.h"

#ifdef __CALYPSI__
    #include <calypsi/intrinsics65816.h>
#endif

// assembly for instructions that can't be directly specified in C
// Always use the compiler defines, not whether inline ASM is enabled
/**
 * @brief Emits a CPU Wait for Interrupt (WAI) instruction.
 * 
 * Halts CPU execution and puts the processor in low-power sleep mode until 
 * the next hardware interrupt (such as VBlank NMI or IRQ) triggers.
 */
void Asm_EmitWai(void) {
#ifdef __VBCC__
    __asm("\twai\n");
#endif
    
#ifdef __CALYPSI__
    __wait_for_interrupt;
#endif
}

/**
 * @brief Emits a CPU Set Interrupt Disable (SEI) instruction.
 * 
 * Sets the interrupt disable flag in the status register, masking maskable hardware interrupts.
 */
void Asm_EmitSei(void) {
#ifdef __VBCC__
    __asm("\tsei\n");
#endif

#ifdef __CALYPSI__
    __disable_interrupts;
#endif
}

/**
 * @brief Emits a CPU Clear Interrupt Disable (CLI) instruction.
 * 
 * Clears the interrupt disable flag in the status register, enabling maskable interrupts.
 */
void Asm_EmitCli(void) {
#ifdef __VBCC__
    __asm("\tcli\n");
#endif

#ifdef __CALYPSI__
    __enable_interrupts;
#endif
}
