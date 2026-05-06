#include <snes/console.h>
#include <stdint.h>

#include "vars.h"

#include "dma.h"
#include "interrupt.h"
#include "interrupt_sub.h"

#ifdef __CALYPSI__ // This should use the compiler define always
    __attribute__((interrupt(0xffea))) void __irq_vblank(void)
#else
    NEAR INTERRUPT void __irq_vblank(void)
#endif
{
    interrupt_vblank_sub();
    
    return;
}
