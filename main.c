#include <snes/console.h>

#include "vars.h"

#include "system.h"
#include "level.h"
#include "loop.h"

#include "ani_pal_hdma.h"

#include "snd.h"

#include "main.h"

#include "sram_management.h"

int main()
{
    system_init_regs();
    system_init_zp(); // Wipe ZP

    rand_array[0] = 1; // Set the seed here

    snd_start(); // start the SPC

    // The SPC will take a while to finish init, do something else in the meantime

    // Check the SRAM contents
    sram_check();

    // Initialize the game
    system_init();

    // Load the level
    level_data_ptr = LEVEL_INITIAL; // Set the initial level here
    level_load(level_data_ptr);

    // Upload SFX data (shared for entire game)
    snd_upload_sample_list((struct sample_list_entry *)&data_snd_samples[0]);

    // Upload instrument and music sequence data
    // TODO: describe a sequence pointer and structure so this can be handled as a single pointer to pass to a function
    snd_upload_instrument_list((struct sample_list_entry_ins *)&data_snd_instruments[0]);
    snd_upload_sequence((struct seq_command *)&data_seq_test_t1[0], 0); // Drum 1
    snd_upload_sequence((struct seq_command *)&data_seq_test_t2[0], 1); // Drum 2
    snd_upload_sequence((struct seq_command *)&data_seq_test_t3[0], 2); // Bass
    snd_upload_sequence((struct seq_command *)&data_seq_test_t4[0], 3); // Secondary
    //snd_upload_sequence((struct seq_command *)&data_seq_test_t5[0], 4); // Drum test sequence
    //snd_upload_sequence((struct seq_command *)&data_seq_test_t6[0], 5); // Drum + instrument test sequence
    snd_set_tempo(120);
    snd_music_play(); 

    ani_pal_hdma_enable();

    system_current_routine = ROUTINE_FADEIN;
    system_target_routine = ROUTINE_GAMELOOP;

    system_setup_tilemap_display(system_target_routine);
    system_init_display(system_target_routine);
    
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