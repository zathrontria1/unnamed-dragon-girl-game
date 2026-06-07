#include <snes/console.h>

#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "system.h"
#include "level.h"
#include "loop.h"
#include "loop_subscreen.h"

#include "hdma.h"

#include "snd.h"

#include "main.h"
#include "dma.h"

#include "sram_management.h"

int main()
{
    system_init_regs(); // Display will be turned off within this
    system_init_wram_functions(); // Write opcodes for WRAM functions
    
    rand_array[0] = 1; // Set the seed here

    system_display_splash(); // A good amount of init is here.
    
    system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_FADEIN);
    system_target_routine = ROUTINE_GAMELOOP;

    system_setup_tilemap_display(system_target_routine);
    system_init_display(system_target_routine);

    HdmaEngine_EnableHdma();

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
            return (void *)&Loop_Game;
            break;
        case ROUTINE_GAMELOOP_RELOAD:
            return (void *)&Loop_Game_ReloadScene;
            break;
        case ROUTINE_PAUSE:
            return (void *)&Loop_Game_Pause;
            break;
        case ROUTINE_NEWLEVEL:
            return (void *)&Loop_Game_NewLevel;
            break;
        case ROUTINE_MAPDISPLAY:
            return (void *)&Loop_Subscreen_MapDisplay;
            break;
        case ROUTINE_MAPDISPLAY_INIT:
            return (void *)&Loop_Subscreen_MapDisplay_Init;
            break;
        case ROUTINE_MSGBOX:
            return (void *)&Loop_Game_Messagebox;
            break;
        case ROUTINE_FADEIN:
            return (void *)&Loop_Fade_In;
            break;
        case ROUTINE_FADEOUT:
            return (void *)&Loop_Fade_Out;
            break;
        case ROUTINE_SUBSCREEN:
            return (void *)&loop_subscreen_top;
            break;
        case ROUTINE_SUBSCREEN_HELP:
            return (void *)&loop_subscreen_help;
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

void __write(){} // Disable the screen printing functions
