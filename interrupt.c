#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "snd.h"
#include "interrupt.h"
#include "interrupt_sub.h"

#ifdef __CALYPSI__ // This should use the compiler define always
    __attribute__((interrupt(0xffea))) void __irq_vblank(void)
#else
    NEAR INTERRUPT void __irq_vblank(void)
#endif
{
    if (system_use_alternate_nmi == 0)
    {
        Nmi_Primary();
    }
    else
    {
        Nmi_Alternate();
    }

    if (snd_stream_enable)
    {
        SoundInterface_NmiAudioUpload();
    }
    
    return;
}
