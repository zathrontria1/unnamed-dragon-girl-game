#include <snes/console.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "main.h"
#include "ui.h"
#include "dma.h"
#include "hdma.h"
#include "system.h"

#include "spr.h"

#include "snd.h"
#include "consts_snd.h"

#include "loop_subscreen.h"

#include "data_strings.h"

bool subscreen_rendered;
uint16_t subscreen_selection;
uint16_t subscreen_bottom_entry;

uint16_t subscreen_cursor_x;
uint16_t subscreen_cursor_y;

void loop_subscreen_top()
{
    system_game_paused = 1;
    system_dont_count_lag = 1;

    hdma_use_gradient = 0xffff;
    hdma_gradient_ptr = (uint16_t)((uint32_t)&hdma_windowbackground_tables[1][0]);

    ui_in_subscreen = 1;

    if (!subscreen_rendered)
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

        UserInterface_ClearWindowBuffer(false);
        UserInterface_ClearTextBuffer();

        UserInterface_DrawWindowBackground(0,0,12,10);
        UserInterface_DrawWindowBackground(6,22,26,6);

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

        UserInterface_DrawWindowText((char *)level_data_ptr->level_name, 7, 23);
        UserInterface_DrawWindowText((char *)&temp_money_string, 7, 25);
        UserInterface_DrawWindowText((char *)&temp_lag_frames, 7, 27);
        
        UserInterface_CopyUiBuffers();

        subscreen_rendered = 1;
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
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
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
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
        }

        int16_t x = subscreen_items_toplevel[subscreen_selection].x;
        int16_t y = subscreen_items_toplevel[subscreen_selection].y;

        SpriteEngine_DrawUISprite(x, y, (0x2c | PAL_SYS_IMPACT << 9 | 3 << 12));

        SpriteEngine_ProcessSpriteLists();

        SpriteEngine_ResetOam();
        SpriteEngine_PackOamHighTable();

        loop_subscreen_top_drawtime();

        bool temp_exit_subscreen = false;

        if (system_check_for_key(KEY_A))
        {
            if (subscreen_items_toplevel[subscreen_selection].action == MENUACTION_OPENSUBSCREEN)
            {
                SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                if (subscreen_items_toplevel[subscreen_selection].ptr != 0)
                {
                    subscreen_rendered = 0;
                    system_loop_func_ptr = subscreen_items_toplevel[subscreen_selection].ptr;

                    return;
                }
                else
                {
                    ;// Pointer is invalid, do nothing
                }
            }
            else if (subscreen_items_toplevel[subscreen_selection].action == MENUACTION_OPENMAPSCREEN)
            {
                // Open the map screen.
                SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);

                shadow_inidisp = 0x0f;
                system_use_alternate_nmi = 1;
                shadow_inidisp_change = -1;

                system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_MAPDISPLAY_INIT);
                system_target_routine = ROUTINE_MAPDISPLAY_INIT;

                return;
            }
            else if (subscreen_items_toplevel[subscreen_selection].action == MENUACTION_CALLFUNCTION)
            {
                SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                if (subscreen_items_toplevel[subscreen_selection].ptr != 0)
                {
                    // Directly call the function without changing the subscreen
                    void (*func)() = subscreen_items_toplevel[subscreen_selection].ptr;
                    func();

                    return;
                }
                else
                {
                    ;// Pointer is invalid, do nothing
                }
            }
            else if (subscreen_items_toplevel[subscreen_selection].action == MENUACTION_EXITSUBSCREEN)
            {
                temp_exit_subscreen = true;
            }
        }

        if (system_check_for_key(KEY_X) || system_check_for_key(KEY_B) || temp_exit_subscreen)
        {
            // Exiting the top level subscreen.
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                
            system_game_paused = 0;

            UserInterface_ClearWindowBuffer(true);
            UserInterface_ClearTextBuffer();
            UserInterface_CopyUiBuffers(); // Perform a total clear

            ui_force_update = 1; // Then perform a wipe of the UI

            ui_in_subscreen = 0;

            system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_GAMELOOP);
            system_target_routine = ROUTINE_GAMELOOP;
        }
    }

    return;
}

void loop_subscreen_profile()
{
    system_game_paused = 1;
    system_dont_count_lag = 1;

    hdma_use_gradient = 0xffff;
    hdma_gradient_ptr = (uint16_t)((uint32_t)&hdma_windowbackground_tables[1][0]);

    ui_in_subscreen = 1;

    if (!subscreen_rendered)
    {
        subscreen_selection = 0;
        subscreen_bottom_entry = 0;

        for (int i = 0; i < 256; i++)
        {
            if ((subscreen_items_profile[i].x == 255) && (subscreen_items_profile[i].y == 255))
            {
                subscreen_bottom_entry = i-1;
                break;
            }
        }
        
        UserInterface_ClearWindowBuffer(false);
        UserInterface_ClearTextBuffer();

        UserInterface_DrawWindowBackground(0,0,32,2);
        UserInterface_DrawWindowBackground(0,2,32,4);
        UserInterface_DrawWindowBackground(0,16,32,12);

        loop_subscreen_profile_drawtext(false);
        
        UserInterface_CopyUiBuffers();
        
        subscreen_rendered = 1;
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
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
            loop_subscreen_help_drawtext(true);
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
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
            loop_subscreen_help_drawtext(true);
        }

        int16_t x = subscreen_items_profile[subscreen_selection].x;
        int16_t y = subscreen_items_profile[subscreen_selection].y;

        SpriteEngine_DrawUISprite(x, y, (0x2c | PAL_SYS_IMPACT << 9 | 3 << 12));

        SpriteEngine_ProcessSpriteLists();

        SpriteEngine_ResetOam();
        SpriteEngine_PackOamHighTable();

        bool temp_exit_subscreen = false;

        if (system_check_for_key(KEY_A))
        {
            if (subscreen_items_profile[subscreen_selection].action == MENUACTION_OPENSUBSCREEN)
            {
                SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                if (subscreen_items_profile[subscreen_selection].ptr != 0)
                {
                    subscreen_rendered = 0;
                    system_loop_func_ptr = subscreen_items_profile[subscreen_selection].ptr;

                    return;
                }
                else
                {
                    ;// Pointer is invalid, do nothing
                }
            }
            else if (subscreen_items_profile[subscreen_selection].action == MENUACTION_CALLFUNCTION)
            {
                SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                if (subscreen_items_profile[subscreen_selection].ptr != 0)
                {
                    // Directly call the function without changing the subscreen
                    void (*func)() = subscreen_items_profile[subscreen_selection].ptr;
                    func();

                    return;
                }
                else
                {
                    ;// Pointer is invalid, do nothing
                }
            }
            else if (subscreen_items_profile[subscreen_selection].action == MENUACTION_EXITSUBSCREEN)
            {
                temp_exit_subscreen = true;
            }
        }

        if (system_check_for_key(KEY_B) || temp_exit_subscreen)
        {
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
            subscreen_rendered = 0;
            // Exiting the profile subscreen.

            system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_SUBSCREEN);
            system_target_routine = ROUTINE_SUBSCREEN;
        }
    }

    return;
}

void loop_subscreen_profile_drawtext(bool copy_result)
{
    UserInterface_ClearTextBuffer();

    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_HEADING, 3, 1);

    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_HEALTH, 1, 3);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_ATTACK, 1, 4);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_DEFENSE, 16, 4);

    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_MONEY, 1, 17);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_UPGRADE_HP, 3, 19);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_COST, 7, 20);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_UPGRADE_ATTACK, 3, 21);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_COST, 7, 22);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_UPGRADE_DEFENSE, 3, 23);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_COST, 7, 24);

    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_RETURN, 3, 26);

    if (copy_result)
    {
        UserInterface_CopyUiBuffers();
    }

    return;
}

void loop_subscreen_help()
{
    system_game_paused = 1;
    system_dont_count_lag = 1;

    hdma_use_gradient = 0xffff;
    hdma_gradient_ptr = (uint16_t)((uint32_t)&hdma_windowbackground_tables[1][0]);

    ui_in_subscreen = 1;

    if (!subscreen_rendered)
    {
        subscreen_selection = 0;
        subscreen_bottom_entry = 0;

        for (int i = 0; i < 256; i++)
        {
            if ((subscreen_items_help[i].x == 255) && (subscreen_items_help[i].y == 255))
            {
                subscreen_bottom_entry = i-1;
                break;
            }
        }
        
        UserInterface_ClearWindowBuffer(false);
        UserInterface_ClearTextBuffer();

        UserInterface_DrawWindowBackground(0,0,32,2);
        UserInterface_DrawWindowBackground(0,2,4,26);
        UserInterface_DrawWindowBackground(4,2,28,26);

        loop_subscreen_help_drawtext(false);
        
        UserInterface_CopyUiBuffers();
        
        subscreen_rendered = 1;
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
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
            loop_subscreen_help_drawtext(true);
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
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
            loop_subscreen_help_drawtext(true);
        }

        int16_t x = subscreen_items_help[subscreen_selection].x;
        int16_t y = subscreen_items_help[subscreen_selection].y;

        SpriteEngine_DrawUISprite(x, y, (0x2c | PAL_SYS_IMPACT << 9 | 3 << 12));

        SpriteEngine_ProcessSpriteLists();

        SpriteEngine_ResetOam();
        SpriteEngine_PackOamHighTable();

        if (system_check_for_key(KEY_B))
        {
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
            subscreen_rendered = 0;
            // Exiting the help subscreen.

            system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_SUBSCREEN);
            system_target_routine = ROUTINE_SUBSCREEN;
        }
    }

    return;
}

/*
    Helper function to draw for Help
*/
void loop_subscreen_help_drawtext(bool copy_result)
{
    UserInterface_ClearTextBuffer();

    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_HEADING, 3, 1);
    
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_MOVEMENT_H, 2, 3);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_INTERACTION_H, 2, 5);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_ATTACK_H, 2, 7);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_PROGRESSION_H, 2, 9);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_MAP_H, 2, 11);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_RESET_H, 2, 13);

    UserInterface_DrawWindowText((char *)subscreen_items_help[subscreen_selection].ptr, 5, 3);

    if (copy_result)
    {
        UserInterface_CopyUiBuffers();
    }

    return;
}

/*
    Helper function to update play time
*/
void loop_subscreen_top_drawtime()
{
    UserInterface_ClearTextBuffer_Line(26);
    char temp_time_string[32] = "          ";
    uint16_t temp_h = (system_frames_elapsed / FPS) / (3600l);
    uint16_t temp_m = ((system_frames_elapsed / FPS) % 3600l) / 60;
    uint16_t temp_s = (system_frames_elapsed / FPS) % 60;

    if ((system_frames_elapsed % (60 / V_MUL)) >= (FPS / 2))
    {
        snprintf((char *)&temp_time_string, 32, (char *)&STR_UI_SUBSCREEN_PLAYTIME_NOCOLON, temp_h, temp_m, temp_s);
    }
    else
    {
        snprintf((char *)&temp_time_string, 32, (char *)&STR_UI_SUBSCREEN_PLAYTIME, temp_h, temp_m, temp_s);
    }
    
    UserInterface_DrawWindowText((char *)&temp_time_string, 7, 26);
    UserInterface_CopyTextBuffer_Line(26);

    return;
}

void loop_subscreen_resetconfirm()
{
    system_game_paused = 1;
    system_dont_count_lag = 1;

    hdma_use_gradient = 0xffff;
    hdma_gradient_ptr = (uint16_t)((uint32_t)&hdma_windowbackground_tables[1][0]);

    ui_in_subscreen = 1;

    if (!subscreen_rendered)
    {
        subscreen_selection = 1;
        subscreen_bottom_entry = 0;

        for (int i = 0; i < 256; i++)
        {
            if ((subscreen_items_resetconfirm[i].x == 255) && (subscreen_items_resetconfirm[i].y == 255))
            {
                subscreen_bottom_entry = i-1;
                break;
            }
        }
        
        UserInterface_ClearWindowBuffer(false);
        UserInterface_ClearTextBuffer();

        UserInterface_DrawWindowBackground(0,10,32,8);

        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_RESETCONFIRMATION, 1, 11);

        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_CONFIRM_YES, 3, 14);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_CONFIRM_NO, 3, 16);
        
        UserInterface_CopyUiBuffers();
        
        subscreen_rendered = 1;
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
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
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
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
        }

        int16_t x = subscreen_items_resetconfirm[subscreen_selection].x;
        int16_t y = subscreen_items_resetconfirm[subscreen_selection].y;

        SpriteEngine_DrawUISprite(x, y, (0x2c | PAL_SYS_IMPACT << 9 | 3 << 12));

        SpriteEngine_ProcessSpriteLists();

        SpriteEngine_ResetOam();
        SpriteEngine_PackOamHighTable();

        bool temp_exit_subscreen = false;

        if (system_check_for_key(KEY_A))
        {
            if (subscreen_items_resetconfirm[subscreen_selection].action == MENUACTION_OPENSUBSCREEN)
            {
                SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                if (subscreen_items_resetconfirm[subscreen_selection].ptr != 0)
                {
                    subscreen_rendered = 0;
                    system_loop_func_ptr = subscreen_items_resetconfirm[subscreen_selection].ptr;

                    return;
                }
                else
                {
                    ;// Pointer is invalid, do nothing
                }
            }
            else if (subscreen_items_resetconfirm[subscreen_selection].action == MENUACTION_CALLFUNCTION)
            {
                SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                if (subscreen_items_resetconfirm[subscreen_selection].ptr != 0)
                {
                    // Directly call the function without changing the subscreen
                    void (*func)() = subscreen_items_resetconfirm[subscreen_selection].ptr;
                    func();

                    return;
                }
                else
                {
                    ;// Pointer is invalid, do nothing
                }
            }
            else if (subscreen_items_resetconfirm[subscreen_selection].action == MENUACTION_EXITSUBSCREEN)
            {
                temp_exit_subscreen = true;
            }
        }

        if (system_check_for_key(KEY_B) || temp_exit_subscreen)
        {
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
            subscreen_rendered = 0;
            // Exiting the confirmation subscreen.

            system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_SUBSCREEN);
            system_target_routine = ROUTINE_SUBSCREEN;
        }
    }

    return;
}

const struct menu_item subscreen_items_toplevel[7] = {
    {6, 16, MENUACTION_EXITSUBSCREEN, 0}, 

    {6, 24, MENUACTION_OPENSUBSCREEN, (void *)&loop_subscreen_profile}, 
    {6, 32, MENUACTION_OPENMAPSCREEN, 0}, 
    {6, 40, MENUACTION_OPENSUBSCREEN, (void *)&loop_subscreen_help}, 
    {6, 48, MENUACTION_OPENSUBSCREEN, 0}, 
    
    {6, 56, MENUACTION_OPENSUBSCREEN, (void *)&loop_subscreen_resetconfirm}, 

    {255, 255, 0, 0}, 
};

const struct menu_item subscreen_items_profile[5] = {
    {6, 152, MENUACTION_EXITSUBSCREEN, 0}, 
    {6, 168, MENUACTION_EXITSUBSCREEN, 0}, 
    {6, 184, MENUACTION_EXITSUBSCREEN, 0}, 

    {6, 208, MENUACTION_EXITSUBSCREEN, 0}, 

    {255, 255, 0, 0}, 
};

const struct menu_item subscreen_items_help[7] = {
    {-2, 24, 0, (void *)&STR_UI_SUBSCREEN_HELP_MOVEMENT}, 
    {-2, 40, 0, (void *)&STR_UI_SUBSCREEN_HELP_INTERACTION}, 
    {-2, 56, 0, (void *)&STR_UI_SUBSCREEN_HELP_ATTACK}, 
    {-2, 72, 0, (void *)&STR_UI_SUBSCREEN_HELP_PROGRESSION}, 
    {-2, 88, 0, (void *)&STR_UI_SUBSCREEN_HELP_MAP}, 
    {-2, 104, 0, (void *)&STR_UI_SUBSCREEN_HELP_RESET}, 

    {255, 255, 0, 0}, 
};

const struct menu_item subscreen_items_resetconfirm[3] = {
    {6, 112, MENUACTION_CALLFUNCTION, (void *)&system_soft_reset}, 
    {6, 128, MENUACTION_EXITSUBSCREEN, 0}, 

    {255, 255, 0, 0}, 
};
