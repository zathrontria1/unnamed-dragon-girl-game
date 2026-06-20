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

    SoundInterface_RunDeferredCommands();
    
    return;
}

/*
    Used for the cutscene player.
*/
#ifdef __CALYPSI__ // This should use the compiler define always
    __attribute__((interrupt(0xffee))) void __irq_ext(void)
#else
    NEAR INTERRUPT void __irq_ext(void)
#endif
{
    Nmi_Cutscene();

    if (snd_stream_enable)
    {
        SoundInterface_NmiAudioUpload();
    }

    SoundInterface_RunDeferredCommands();
    
    register volatile char temp = REG_TIMEUP;
    return;
}
