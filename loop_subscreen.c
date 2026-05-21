#include <snes/console.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "main.h"
#include "ui.h"
#include "dma.h"
#include "system.h"

#include "spr.h"

#include "snd.h"
#include "consts_snd.h"

#include "loop_subscreen.h"

#include "data_strings.h"

bool subscreen_toplevel_rendered;
uint16_t subscreen_selection;
uint16_t subscreen_bottom_entry;

uint16_t subscreen_cursor_x;
uint16_t subscreen_cursor_y;

void loop_subscreen_top()
{
    system_game_paused = 1;
    system_dont_count_lag = 1;

    if (!subscreen_toplevel_rendered)
    {
        subscreen_selection = 0;
        subscreen_bottom_entry = 0;

        for (int i = 0; i < 256; i++)
        {
            if ((subscreen_items_toplevel[i].x == 255) && (subscreen_items_toplevel[i].y == 255))
            {
                subscreen_bottom_entry = i-1;
                break;
            }
        }

        UserInterface_ClearWindowBuffer();
        UserInterface_ClearTextBuffer();

        UserInterface_DrawWindowBackground(0,0,12,10);
        UserInterface_DrawWindowBackground(6,22,26,4);

        char temp_money_string[32] = "          ";
        
        snprintf((char *)&temp_money_string, 32, (char *)&STR_UI_SUBSCREEN_MONEY, obj_player_pointer->struct_data.npc_data.money);

        loop_subscreen_top_drawtime();

        char temp_lag_frames[32] = "          ";
        snprintf((char *)&temp_lag_frames, 32, (char *)&STR_UI_SUBSCREEN_LAGCOUNTER, system_frames_lag);

        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_RESUME, 3, 2);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE, 3, 3);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_MAP, 3, 4);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP, 3, 5);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS, 3, 6);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_RESTART, 3, 7);

        UserInterface_DrawWindowText((char *)&temp_money_string, 7, 23);
        UserInterface_DrawWindowText((char *)&temp_lag_frames, 7, 25);
        
        UserInterface_CopyUiBuffers();

        subscreen_toplevel_rendered = 1;
    }
    else
    {
        // Perform menu navigation
        if (system_check_for_key(KEY_UP))
        {
            if (subscreen_selection == 0)
            {
                subscreen_selection = subscreen_bottom_entry;
            }
            else
            {
                subscreen_selection--;
            }
        }
        else if (system_check_for_key(KEY_DOWN))
        {
            if (subscreen_selection >= subscreen_bottom_entry)
            {
                subscreen_selection = 0;
            }
            else
            {
                subscreen_selection++;
            }
        }

        int16_t x = subscreen_items_toplevel[subscreen_selection].x;
        int16_t y = subscreen_items_toplevel[subscreen_selection].y;

        SpriteEngine_DrawUISprite(x, y, (0x2c | PAL_SYS_IMPACT << 9 | 3 << 12));

        SpriteEngine_ProcessSpriteLists();

        SpriteEngine_ResetOam();
        SpriteEngine_PackOamHighTable();

        loop_subscreen_top_drawtime();

        if (system_check_for_key(KEY_X) || system_check_for_key(KEY_B))
        {
            // Exiting the top level subscreen.
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                
            system_game_paused = 0;

            UserInterface_ClearWindowBuffer();
            UserInterface_ClearTextBuffer();
            UserInterface_CopyUiBuffers(); // Perform a total clear

            ui_force_update = 1; // Then perform a wipe of the UI

            system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_GAMELOOP);
            system_target_routine = ROUTINE_GAMELOOP;
        }
    }

    return;
}

/*
    Helper function to update play time
*/
void loop_subscreen_top_drawtime()
{
    UserInterface_ClearTextBuffer_Line(24);
    char temp_time_string[32] = "          ";
    uint16_t temp_h = (uint16_t)((system_frames_elapsed / FPS) / (3600l));
    uint16_t temp_m = (uint16_t)(((system_frames_elapsed / FPS) % (3600l)) / 60);
    uint16_t temp_s = (uint16_t)((system_frames_elapsed / FPS) % 60);

    if ((system_frames_elapsed % (60 / V_MUL)) >= (FPS / 2))
    {
        snprintf((char *)&temp_time_string, 32, (char *)&STR_UI_SUBSCREEN_PLAYTIME_NOCOLON, temp_h, temp_m, temp_s);
    }
    else
    {
        snprintf((char *)&temp_time_string, 32, (char *)&STR_UI_SUBSCREEN_PLAYTIME, temp_h, temp_m, temp_s);
    }
    
    UserInterface_DrawWindowText((char *)&temp_time_string, 7, 24);
    UserInterface_CopyTextBuffer_Line(24);

    return;
}

const struct menu_item subscreen_items_toplevel[7] = {
    {8, 12, MENUACTION_EXITSUBSCREEN, 0}, 

    {8, 20, MENUACTION_OPENSUBSCREEN, 0}, 
    {8, 28, MENUACTION_OPENSUBSCREEN, 0}, 
    {8, 36, MENUACTION_OPENSUBSCREEN, 0}, 
    {8, 44, MENUACTION_OPENSUBSCREEN, 0}, 
    
    {8, 52, MENUACTION_CALLFUNCTION, (void *)&system_reset}, 

    {255, 255, 0, 0}, 
};