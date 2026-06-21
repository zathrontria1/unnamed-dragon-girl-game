#include <snes/console.h>

#include <stdlib.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "system.h"
#include "level.h"
#include "loop.h"
#include "loop_subscreen.h"

#include "loop_cutscene.h"

#include "hdma.h"

#include "math_int.h"
#include "snd.h"

#include "main.h"
#include "dma.h"

#include "sram_management.h"

#include "errorhandling.h"

int main()
{
    System_Init_CpuRegs(); // Display will be turned off within this
    System_Init_WramFunctions(); // Write opcodes for WRAM functions
    
    rand_array[0] = 1; // Set the seed here

    // Check controller validty. If it's the wrong type load an error message
    if (System_CheckController() != 0x0000)
    {
        ErrorHandler_Controller();
    }

    System_DisplayStartupSplash(); // A good amount of init is here.
    
    system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_CUTSCENE_INIT);
    //system_target_routine = ROUTINE_GAMELOOP;

    // WIP: attempt to integrate the cutscene engine
    system_target_routine = ROUTINE_CUTSCENE_INIT;

    cs_current = (struct cutscene_data *)&data_cs_intro;
    
    System_Init_TilemapSettings(system_target_routine);
    System_Init_DisplaySettings(system_target_routine);
    
    System_EnableInterrupts();
    
    while (1)
    {   
        System_WaitUntilVblank();

        void (*func)() = system_loop_func_ptr;
        func();

        HdmaEngine_SetHdmaShadow();
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
        case ROUTINE_CUTSCENE:
            return (void *)&CsEngine_Loop;
            break;
        case ROUTINE_CUTSCENE_INIT:
            return (void *)&CsEngine_StartCutscene;
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
    System_Reset();

    // Unreachable

    return;
}

void __write(){} // Disable the screen printing functions
