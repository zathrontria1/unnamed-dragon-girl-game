#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "snd.h"
#include "interrupt.h"
#include "interrupt_sub.h"

#include "crash_handler.h"

/**
 * @brief Default VBlank Interrupt Vector Handler (NMI).
 * 
 * Invoked automatically during vertical blanking. Checks the active screen state 
 * to execute either the primary or alternate NMI loop, sends pending audio commands, 
 * then uploads audio data to the SPC700 if there is audio streaming enabled.
 */
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

/**
 * @brief External Interrupt Vector Handler (V-IRQ).
 * 
 * Used for starting a Vblank routine when V-IRQ is used instead of NMI.
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

/**
 * @brief Software Break (BRK) Vector Handler (Native Mode).
 * 
 * Invoked upon CPU execution of a BRK software interrupt instruction. Redirects 
 * execution to the crash handler interface.
 */
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

/**
 * @brief Co-Processor (COP) Vector Handler (Native Mode).
 * 
 * Triggered by the COP assembly instruction. Diverts execution to the crash handler.
 */
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

/**
 * @brief Co-Processor (COP) Vector Handler (6502 Emulation Mode).
 * 
 * Triggered by COP in emulation mode. Diverts execution to the emulation crash handler.
 */
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

/**
 * @brief External Interrupt/Break Vector Handler (6502 Emulation Mode).
 * 
 * Triggered by IRQ/BRK in emulation mode. Diverts execution to the emulation crash handler.
 */
#ifdef __CALYPSI__
    __attribute__((interrupt(0xfffe))) void __irq_ext6502(void);
#else
    NEAR INTERRUPT void __irq_ext6502(void)
{
    System_CrashHandler_EmulationMode();
}
#endif

