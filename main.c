#include <snes/console.h>

#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "system.h"
#include "level.h"
#include "loop.h"

#include "ani_pal_hdma.h"

#include "snd.h"

#include "main.h"
#include "dma.h"

#include "sram_management.h"

int main()
{
    system_init_regs(); // Display will be turned off within this
    //system_init_zp(); // Wipe ZP // Now done within startup
    system_init_wram_functions(); // Write opcodes for WRAM functions
    
    rand_array[0] = 1; // Set the seed here

    system_display_splash(); // A good amount of init is here.

    system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_FADEIN);
    //system_current_routine = ROUTINE_FADEIN;
    system_target_routine = ROUTINE_GAMELOOP;

    system_setup_tilemap_display(system_target_routine);
    system_init_display(system_target_routine);

    ani_pal_hdma_enable();

    SoundInterface_PlayMusic(); 
    
    system_interrupt_enable();
    
    while (1)
    {   
        system_wait_vblank();

        void (*func)() = system_loop_func_ptr;
        func();
    }

    return 0;
}

void * main_GetFunctionPointer(uint16_t routine)
{
    system_current_routine = routine; // Also set this for code that don't want to read function pointers

    switch (routine)
    {
        case ROUTINE_GAMELOOP:
            return (void *)&loop_game;
            break;
        case ROUTINE_GAMELOOP_RELOAD:
            return (void *)&loop_game_reload;
            break;
        case ROUTINE_PAUSE:
            return (void *)&loop_pause;
            break;
        case ROUTINE_NEWLEVEL:
            return (void *)&loop_game_newlevel;
            break;
        case ROUTINE_MAPDISPLAY:
            return (void *)&loop_mapdisplay;
            break;
        case ROUTINE_MAPDISPLAY_INIT:
            return (void *)&loop_mapdisplay_init;
            break;
        case ROUTINE_MSGBOX:
            return (void *)&loop_messagebox;
            break;
        case ROUTINE_FADEIN:
            return (void *)&loop_fadein;
            break;
        case ROUTINE_FADEOUT:
            return (void *)&loop_fadeout;
            break;
        case ROUTINE_RESET: // Special cased to immediately reset
            return (void *)&main_Reset;
            break;
    }

    return NULL;
}

/*
    Call to soft reset.
*/
void main_Reset()
{
    SoundInterface_ResetAPU();
    system_reset();

    // Unreachable

    return;
}