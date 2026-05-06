#include <snes/console.h>

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
    system_init_regs();
    system_init_zp(); // Wipe ZP

    REG_INIDISP = 0x8f;

    rand_array[0] = 1; // Set the seed here

    system_display_splash(); // A good amount of init is here.

    system_current_routine = ROUTINE_FADEIN;
    system_target_routine = ROUTINE_GAMELOOP;

    system_setup_tilemap_display(system_target_routine);
    system_init_display(system_target_routine);

    ani_pal_hdma_enable();

    snd_music_play(); 
    
    system_interrupt_enable();
    
    while (1)
    {   
        switch (system_current_routine)
        {
            case ROUTINE_GAMELOOP:
                loop_game();
                break;
            case ROUTINE_GAMELOOP_RELOAD:
                loop_game_reload();
                break;
            case ROUTINE_PAUSE:
                loop_pause();
                break;
            case ROUTINE_MAPDISPLAY:
                loop_mapdisplay();
                break;
            case ROUTINE_MAPDISPLAY_INIT:
                loop_mapdisplay_init();
                break;
            case ROUTINE_MSGBOX:
                loop_messagebox();
                break;
            case ROUTINE_FADEIN:
                loop_fadein();
                break;
            case ROUTINE_FADEOUT:
                loop_fadeout();
                break;
            case ROUTINE_RESET:
                snd_reset();
                system_reset();
                break;
        }
    }

    return 0;
}