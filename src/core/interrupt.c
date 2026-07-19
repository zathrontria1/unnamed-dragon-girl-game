#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "snd.h"
#include "interrupt.h"
#include "interrupt_sub.h"

#include "crash_handler.h"

#ifdef __CALYPSI__ // This should use the compiler define always
    __attribute__((interrupt(0xffea))) void __irq_vblank(void)
#else
    NEAR INTERRUPT void __irq_vblank(void)
#endif
{
    if (!system_use_alternate_nmi)
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


#ifdef __CALYPSI__
    __attribute__((interrupt(0xffe6))) void __irq_brk(void)
#else
NEAR INTERRUPT void __irq_brk(void)
#endif
{
    System_CrashHandler();

    // unreachable
    return;
}

#ifdef __CALYPSI__
    __attribute__((interrupt(0xffe4))) void __irq_cop(void)
#else
NEAR INTERRUPT void __irq_cop(void)
#endif
{
    System_CrashHandler();

    // unreachable
    return;
}

#ifdef __CALYPSI__
    __attribute__((interrupt(0xfff4))) void __irq_cop6502(void);
#else
NEAR INTERRUPT void __irq_cop6502(void)
{
    System_CrashHandler_EmulationMode();

    // unreachable
    return;
}
#endif

#ifdef __CALYPSI__
    __attribute__((interrupt(0xfffe))) void __irq_ext6502(void);
#else
NEAR INTERRUPT void __irq_ext6502(void)
{
    System_CrashHandler_EmulationMode();
}
#endif
