#include "snes/console.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "obj.h"
#include "level.h"

#include "main.h"
#include "ui.h"
#include "dma.h"
#include "hdma.h"
#include "system.h"

#include "spr.h"
#include "gfx.h"

#include "snd.h"
#include "consts_snd.h"

#include "loop.h"
#include "loop_subscreen.h"

#include "data_strings.h"

#include "math_int.h"
#include "lz4.h"

#include "ani_pal.h"

uint16_t subscreen_selection;
uint16_t subscreen_selection_profile;
uint16_t subscreen_bottom_entry;

bool subscreen_is_in_profile;
bool subscreen_restore_sprite_page;

bool subscreen_rendered;
bool subscreen_skip_window_redraw;

uint8_t subscreen_cgadsub_copy;

static void * subscreen_next_func_ptr;
uint16_t subscreen_transition_state;

static uint8_t subscreen_options_volume_repeat_timer;
static uint32_t subscreen_options_volume_repeat_frame;
static uint8_t subscreen_audio_test_timer;

void Loop_Subscreen_Transition_Init()
{
    subscreen_transition_state = TRANSITION_STATE_INITIALIZE;

    while (shadow_brightness != 0x00)
    {
        while ((REG_HVBJOY & VBL_READY) != VBL_READY)
        {
            ;
        }
        DmaSystem_UploadCgram(); // Avoid stale colour palettes. Do it only once per frame
        while ((REG_HVBJOY & VBL_READY) == VBL_READY)
        {
            ;
        }
        // Wait for fade out via alternate NMI
    }

    subscreen_rendered = 0;

    void (*next_func)() = (void (*)())subscreen_next_func_ptr;

    if (subscreen_next_func_ptr == Main_GetFunctionPointer(ROUTINE_GAMELOOP))
    {
        ;//Loop_Game_Partial();
    }
    else
    {
        next_func();
    }

    system_use_alternate_nmi = false;
    shadow_brightness_change = 0;

    subscreen_transition_state = TRANSITION_STATE_FADE_IN;
    system_loop_func_ptr = (void *)&Loop_Subscreen_Transition_FadeIn;

    return;
}

void Loop_Subscreen_Transition_FadeIn()
{
    void (*next_func)() = (void (*)())subscreen_next_func_ptr;

    if (subscreen_next_func_ptr == Main_GetFunctionPointer(ROUTINE_GAMELOOP))
    {
        ;//Loop_Game_Partial();
    }
    else
    {
        next_func();
    }

    shadow_brightness += (128 * V_MUL);
    
    if (shadow_brightness >= (15 << 8))
    {
        shadow_brightness = (15 << 8);
        system_loop_func_ptr = subscreen_next_func_ptr;
        subscreen_transition_state = TRANSITION_STATE_NONE;
    }

    return;
}

void Subscreen_Transition_Start(void * next_func)
{
    subscreen_next_func_ptr = next_func;

    shadow_brightness = 15 << 8;
    shadow_brightness_change = -(128 * V_MUL);

    system_use_alternate_nmi = true;
    subscreen_transition_state = TRANSITION_STATE_FADE_OUT;

    system_loop_func_ptr = (void *)&Loop_Subscreen_Transition_Init;

    return;
}

void Subscreen_Transition_Exit()
{
    DmaSystem_ResetQueue();
    
    // Disable HDMA gradients
    hdma_use_gradient = 0x0000;
    
    // Restore CGADSUB
    shadow_cgadsub = subscreen_cgadsub_copy; 

    // Exiting the subscreen. Load normal player palettes and resume game.
    AniSystem_Pal_LoadSubpalette((uint8_t *)&data_palette_player, 8);

    UserInterface_ClearWindowBuffer(true);
    UserInterface_ClearTextBuffer();
    UserInterface_CopyUiBuffers(); // Perform a total clear

    ui_force_update = true; // Then perform a wipe of the UI

    ui_in_subscreen = false;

    system_target_routine = ROUTINE_GAMELOOP;
    subscreen_next_func_ptr = Main_GetFunctionPointer(ROUTINE_GAMELOOP);

    // Run first frame of gameplay loop while black to populate OAM shadow
    Loop_Game_Partial();

    system_game_paused = false;

    return;
}

void Subscreen_Top()
{
    // Temporarily make a copy of CGADSUB.
    subscreen_cgadsub_copy = shadow_cgadsub;
    shadow_cgadsub = 0x00; // Disable colour math

    Subscreen_Internal_InitState();

    // Silence the fire noise
    if (snd_flame_playing == 1)
    {
        SoundInterface_StopSfx(SFX_ATK_FIRE_BREATH);
        snd_flame_playing = 0;
    }

    if (!subscreen_rendered)
    {
        AniSystem_Pal_LoadSubpalette((uint8_t *)&data_palette_player_portrait, 8);

        subscreen_selection = 0;
        Subscreen_Internal_FindBottomEntry((const struct menu_item *)&subscreen_items_toplevel);

        SpriteEngine_ProcessSpriteLists();

        UserInterface_ClearWindowBuffer(false);
        UserInterface_ClearTextBuffer();

        UserInterface_DrawWindowBackground(0,0,12,10);
        UserInterface_DrawWindowBackground(6,22,26,6);

        char temp_money_string[32] = "          ";
        
        snprintf((char *)&temp_money_string, 32, (char *)&STR_UI_SUBSCREEN_MONEY, obj_player_pointer->struct_data.npc_data.money);

        Subscreen_Top_DrawTime();

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
        if (subscreen_restore_sprite_page)
        {
            if (snd_stream_enable)
            {
                SoundInterface_PauseStream();

                Subscreen_Internal_RestoreLastSpritePage();
                subscreen_restore_sprite_page = false;

                SoundInterface_ResumeStream();
            }
            else
            {
                Subscreen_Internal_RestoreLastSpritePage();
                subscreen_restore_sprite_page = false;
            }
        }

        // Perform menu navigation
        Subscreen_Internal_UpdateNavigation((const struct menu_item *)&subscreen_items_toplevel);

        SpriteEngine_ProcessSpriteLists();

        Subscreen_Top_DrawTime();

        uint16_t action_res = Subscreen_Internal_HandleMenuAction((const struct menu_item *)&subscreen_items_toplevel);
        if (action_res == SUBSCREEN_ACTIONSTATE_RETURN)
        {
            return;
        }

        bool temp_exit_subscreen = (action_res == SUBSCREEN_ACTIONSTATE_EXIT);

        if (System_CheckKey(KEY_X) || System_CheckKey(KEY_B) || temp_exit_subscreen)
        {
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
            Subscreen_Transition_Start((void *)&Subscreen_Transition_Exit);
        }
    }

    return;
}

void Subscreen_Upgrade()
{
    Subscreen_Internal_InitState();

    if (!subscreen_rendered)
    {
        subscreen_selection = subscreen_selection_profile;
        Subscreen_Internal_FindBottomEntry((const struct menu_item *)&subscreen_items_profile);

        // Copy the contents of the last 8KB of VRAM to WRAM first
        // Wait for a transition from non-vblank to vblank.

        if (!subscreen_is_in_profile)
        {
            if (snd_stream_enable)
            {
                SoundInterface_PauseStream();

                Subscreen_Internal_SaveLastSpritePage();

                // Then copy the player character's portrait
                Subscreen_Upgrade_UploadProfilePicture();

                SoundInterface_ResumeStream();
            }
            else
            {
                Subscreen_Internal_SaveLastSpritePage();

                // Then copy the player character's portrait
                Subscreen_Upgrade_UploadProfilePicture();
            }

            subscreen_is_in_profile = true;
        }

        if (!subscreen_skip_window_redraw)
        {
            UserInterface_ClearWindowBuffer(false);

            UserInterface_DrawWindowBackground(0,0,16,3);
            UserInterface_DrawWindowBackground(0,3,16,6);
            UserInterface_DrawWindowBackground(0,16,32,12);

            UserInterface_DrawWindowBox(16,0,16,16);
        }
        
        UserInterface_ClearTextBuffer();

        Subscreen_Upgrade_CalculateUpgradeCosts();

        Subscreen_Upgrade_DrawText(false);
        
        UserInterface_CopyUiBuffers();
        
        subscreen_rendered = 1;
    }
    else
    {
        // Perform menu navigation
        Subscreen_Internal_UpdateNavigation((const struct menu_item *)&subscreen_items_profile);

        // This used to be SpriteEngine_DrawUISprite_Large, but since it's only really used here
        // it's done this way to speed things along.
        struct spr_queue_entry s;
        s.signsize = 0x80; // Large sprite (32x32), positive X position
        
        int16_t y = 0;
        uint16_t base_tileattrib = 0x3100;
        for (int py = 0; py < 4; py++)
        {
            int16_t x = 128;
            uint16_t tileattrib = base_tileattrib;
            s.y = y;
            for (int px = 0; px < 4; px++)
            {
                s.x = x;
                s.tileattrib = tileattrib;
                SpriteEngine_DrawSprite(&s);
                x += 32;
                tileattrib += 4;
            }
            y += 32;
            base_tileattrib += 64;
        }

        SpriteEngine_ProcessSpriteLists();

        uint16_t action_res = Subscreen_Internal_HandleMenuAction((const struct menu_item *)&subscreen_items_profile);
        if (action_res == SUBSCREEN_ACTIONSTATE_RETURN)
        {
            return;
        }

        bool temp_exit_subscreen = (action_res == SUBSCREEN_ACTIONSTATE_EXIT);

        if (System_CheckKey(KEY_B) || temp_exit_subscreen)
        {
            subscreen_restore_sprite_page = true;
            subscreen_is_in_profile = false;
            subscreen_skip_window_redraw = false;

            subscreen_selection_profile = 0;
            
            Subscreen_Internal_GoBackToTop();
        }
    }

    return;
}

/*
    Save the original sprite page
*/
void Subscreen_Internal_SaveLastSpritePage()
{
    // TODO: This is very hacky. Consider making a third NMI routine for this so APU playback isn't a factor.
    if (snd_stream_enable)
    {
        for (int i = 0; i < 8; i++)
        {
            System_AlignToVblank();

            DmaSystem_CopyFromVramToWram(0x7000 + (i << 9), (uint8_t *)0x007fe000 + (i << 10), 1024);
        }
    }
    else
    {
        for (int i = 0; i < 2; i++)
        {
            System_AlignToVblank();

            DmaSystem_CopyFromVramToWram(0x7000 + (i << 11), (uint8_t *)0x007fe000 + (i << 12), 4096);
        }
    }

    return;
}

/*
    Upload the player character's image to VRAM
*/
void Subscreen_Upgrade_UploadProfilePicture()
{
    // First decompress the image
    LZ4_UnpackToWRAM((void *)&data_spr_player_portrait_lz4, (void *)(LZ4_BUFFER_ADDR+0x6000));

    // TODO: This is very hacky. Consider making a third NMI routine for this so APU playback isn't a factor.
    if (snd_stream_enable)
    {
        for (int i = 0; i < 8; i++)
        {
            System_AlignToVblank();

            DmaSystem_CopyToVram((uint8_t *)LZ4_BUFFER_ADDR+0x6000 + (i * 0x0400), 0x7000+(i * 0x200), 1024);
        }
    }
    else
    {
        for (int i = 0; i < 2; i++)
        {
            System_AlignToVblank();

            DmaSystem_CopyToVram((uint8_t *)LZ4_BUFFER_ADDR+0x6000 + (i * 0x1000), 0x7000+(i * 0x800), 4096);
        }
    }

    return;
}

/*
    Restore the original sprite page
*/
void Subscreen_Internal_RestoreLastSpritePage()
{
    // TODO: This is very hacky. Consider making a third NMI routine for this so APU playback isn't a factor.
    if (snd_stream_enable)
    {
        for (int i = 0; i < 8; i++)
        {
            System_AlignToVblank();

            DmaSystem_CopyToVram((uint8_t *)0x007fe000 + (i * 0x0400), 0x7000 + (i * 0x200), 1024);
        }
    }
    else
    {
        for (int i = 0; i < 2; i++)
        {
            System_AlignToVblank();

            DmaSystem_CopyToVram((uint8_t *)0x007fe000 + (i * 0x1000), 0x7000 + (i * 0x800), 4096);
        }
    }

    return;
}

void Subscreen_Upgrade_DrawText(bool copy_result)
{
    UserInterface_ClearTextBuffer();

    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_HEADING, 3, 1);

    char temp_string[32] = "";    
    int temp_hp_offset = snprintf((char *)&temp_string, 32, (char *)&STR_UI_SUBSCREEN_PROFILE_HEALTH, obj_player_pointer->struct_data.npc_data.hp);

    UserInterface_DrawWindowText((char *)&temp_string, 1, 4);

    snprintf((char *)&temp_string, 32, (char *)&STR_UI_SUBSCREEN_PROFILE_HEALTH_DIV, obj_player_pointer->struct_data.npc_data.hp_max);
    UserInterface_DrawWindowText((char *)&temp_string, temp_hp_offset+1, 4);

    snprintf((char *)&temp_string, 32, (char *)&STR_UI_SUBSCREEN_PROFILE_ATTACK, obj_player_pointer->struct_data.npc_data.attack);
    UserInterface_DrawWindowText((char *)&temp_string, 1, 6);

    snprintf((char *)&temp_string, 32, (char *)&STR_UI_SUBSCREEN_PROFILE_DEFENSE, obj_player_pointer->struct_data.npc_data.defense);
    UserInterface_DrawWindowText((char *)&temp_string, 1, 7);

    snprintf((char *)&temp_string, 32, (char *)&STR_UI_SUBSCREEN_PROFILE_MONEY, obj_player_pointer->struct_data.npc_data.money);
    UserInterface_DrawWindowText((char *)&temp_string, 1, 17);

    // Cost sections
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_UPGRADE_HP, 3, 19);
    snprintf((char *)&temp_string, 32, (char *)&STR_UI_SUBSCREEN_PROFILE_COST, obj_player_upgrades_cost_hp);
    UserInterface_DrawWindowText((char *)&temp_string, 7, 20);

    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_UPGRADE_ATTACK, 3, 21);
    snprintf((char *)&temp_string, 32, (char *)&STR_UI_SUBSCREEN_PROFILE_COST, obj_player_upgrades_cost_attack);
    UserInterface_DrawWindowText((char *)&temp_string, 7, 22);

    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_UPGRADE_DEFENSE, 3, 23);
    snprintf((char *)&temp_string, 32, (char *)&STR_UI_SUBSCREEN_PROFILE_COST, obj_player_upgrades_cost_defense);
    UserInterface_DrawWindowText((char *)&temp_string, 7, 24);

    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_PROFILE_RETURN, 3, 26);

    if (copy_result)
    {
        UserInterface_CopyUiBuffers();
    }

    return;
}

/*
    Calculate upgrade costs
*/
void Subscreen_Upgrade_CalculateUpgradeCosts()
{
    obj_player_upgrades_cost_hp = data_upgrade_costs[obj_player_upgrades_bought_hp];
    obj_player_upgrades_cost_attack = data_upgrade_costs[obj_player_upgrades_bought_attack];
    obj_player_upgrades_cost_defense = data_upgrade_costs[obj_player_upgrades_bought_defense];

    return;
}

void Subscreen_Upgrade_Hp()
{
    if (obj_player_pointer->struct_data.npc_data.money >= obj_player_upgrades_cost_hp)
    {
        obj_player_pointer->struct_data.npc_data.hp_max += 10;
        obj_player_pointer->struct_data.npc_data.hp = obj_player_pointer->struct_data.npc_data.hp_max;

        obj_player_pointer->struct_data.npc_data.money -= obj_player_upgrades_cost_hp;
        ui_display_money = obj_player_pointer->struct_data.npc_data.money;

        if (obj_player_upgrades_bought_hp < 255)
        {
            obj_player_upgrades_bought_hp++;
        }

        obj_player_health_regen_limit = obj_player_pointer->struct_data.npc_data.hp_max >> PLAYER_HEALTH_REGEN_LIMITSHIFT;

        subscreen_selection_profile = subscreen_selection;

        subscreen_skip_window_redraw = true;

        subscreen_rendered = 0;

        //SoundInterface_PlayClip(STREAM_VOICE_UPGRADE_SUCCESS_1 + (Math_GetRandom_u16() & 0x01));
    }

    return;
}

void Subscreen_Upgrade_Attack()
{
    if (obj_player_pointer->struct_data.npc_data.money >= obj_player_upgrades_cost_attack)
    {
        obj_player_pointer->struct_data.npc_data.attack += 1;

        obj_player_pointer->struct_data.npc_data.money -= obj_player_upgrades_cost_attack;
        ui_display_money = obj_player_pointer->struct_data.npc_data.money;

        if (obj_player_upgrades_bought_attack < 255)
        {
            obj_player_upgrades_bought_attack++;
        }

        subscreen_selection_profile = subscreen_selection;

        subscreen_skip_window_redraw = true;

        subscreen_rendered = 0;

        //SoundInterface_PlayClip(STREAM_VOICE_UPGRADE_SUCCESS_1 + (Math_GetRandom_u16() & 0x01));
    }

    return;
}

void Subscreen_Upgrade_Defense()
{
    if (obj_player_pointer->struct_data.npc_data.money >= obj_player_upgrades_cost_defense)
    {
        obj_player_pointer->struct_data.npc_data.defense += 1;

        obj_player_pointer->struct_data.npc_data.money -= obj_player_upgrades_cost_defense;
        ui_display_money = obj_player_pointer->struct_data.npc_data.money;

        if (obj_player_upgrades_bought_defense < 255)
        {
            obj_player_upgrades_bought_defense++;
        }

        subscreen_selection_profile = subscreen_selection;

        subscreen_skip_window_redraw = true;

        subscreen_rendered = 0;

        //SoundInterface_PlayClip(STREAM_VOICE_UPGRADE_SUCCESS_1 + (Math_GetRandom_u16() & 0x01));
    }

    return;
}

void Subscreen_Help()
{
    Subscreen_Internal_InitState();

    if (!subscreen_rendered)
    {
        subscreen_selection = 0;
        Subscreen_Internal_FindBottomEntry((const struct menu_item *)&subscreen_items_help);
        
        UserInterface_ClearWindowBuffer(false);
        UserInterface_ClearTextBuffer();

        UserInterface_DrawWindowBackground(0,0,32,3);
        UserInterface_DrawWindowBackground(0,3,4,25);
        UserInterface_DrawWindowBackground(4,3,28,25);

        Subscreen_Help_DrawText(false);
        
        UserInterface_CopyUiBuffers();
        
        subscreen_rendered = 1;
    }
    else
    {
        // Perform menu navigation
        bool update_text = false;

        Subscreen_Internal_UpdateNavigation((const struct menu_item *)&subscreen_items_help);

        if (System_CheckKey(KEY_UP))
        {
            update_text = true;
        }
        else if (System_CheckKey(KEY_DOWN))
        {
            update_text = true;
        }

        if (update_text)
        {
            Subscreen_Help_DrawText(true);
        }

        SpriteEngine_ProcessSpriteLists();

        if (System_CheckKey(KEY_B))
        {
            Subscreen_Internal_GoBackToTop();
        }
    }

    return;
}

/*
    Helper function to draw for Help
*/
void Subscreen_Help_DrawText(bool copy_result)
{
    UserInterface_ClearTextBuffer();

    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_HEADING, 3, 1);
    
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_MOVEMENT_H, 2, 4);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_INTERACTION_H, 2, 6);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_ATTACK_H, 2, 8);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_PROGRESSION_H, 2, 10);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_MAP_H, 2, 12);
    UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_HELP_RESET_H, 2, 14);

    UserInterface_DrawWindowText((char *)subscreen_items_help[subscreen_selection].ptr, 5, 4);

    if (copy_result)
    {
        UserInterface_CopyUiBuffers();
    }

    return;
}

void Subscreen_Options_DrawTile(uint16_t x, uint16_t y, uint16_t tile)
{
    if ((x >= 32) || (y >= (SCREEN_HEIGHT >> 3)))
    {
        return;
    }

    ui_window_text[y][x] = tile | 0x2000 | (PAL_UI_TEXT_WHITE << 10);

    return;
}

static void Subscreen_Options_DrawVolumeBar(uint16_t row, uint8_t vol)
{
    uint8_t volume_display = vol;
    uint8_t volume_full_tiles;
    uint8_t volume_partial_fill;

    if (volume_display != 0)
    {
        volume_display++;
    }
    volume_full_tiles = volume_display >> 3;
    volume_partial_fill = volume_display & 0x07;

    Subscreen_Options_DrawTile(13, row, 0x0062);
    for (uint8_t i = 0; i < 16; i++)
    {
        uint16_t tile = 0x0063;
        if (i < volume_full_tiles)
        {
            tile = 0x006b;
        }
        else if (i == volume_full_tiles)
        {
            tile += volume_partial_fill;
        }
        Subscreen_Options_DrawTile(14 + i, row, tile);
    }
    Subscreen_Options_DrawTile(30, row, 0x006c);

    return;
}

void Subscreen_Options_DrawValues(bool copy_result)
{
    uint16_t checked_tile = 0x006d;
    uint16_t unchecked_tile = 0x006e;

    Subscreen_Options_DrawTile(16, 5, snd_settings_mono ? unchecked_tile : checked_tile);
    Subscreen_Options_DrawTile(25, 5, snd_settings_mono ? checked_tile : unchecked_tile);

    Subscreen_Options_DrawVolumeBar(7, snd_settings_volume_master);
    Subscreen_Options_DrawVolumeBar(9, snd_settings_volume_bgm);
    Subscreen_Options_DrawVolumeBar(11, snd_settings_volume_sfx);
    Subscreen_Options_DrawVolumeBar(13, snd_settings_volume_voice);

    Subscreen_Options_DrawTile(18, 15, gfx_enable_hitblur ? checked_tile : unchecked_tile);
    Subscreen_Options_DrawTile(25, 15, gfx_enable_hitblur ? unchecked_tile : checked_tile);
    Subscreen_Options_DrawTile(18, 17, gfx_enable_heatwave ? checked_tile : unchecked_tile);
    Subscreen_Options_DrawTile(25, 17, gfx_enable_heatwave ? unchecked_tile : checked_tile);

    if (copy_result)
    {
        UserInterface_CopyUiBuffers();
    }

    return;
}

/*
    Options menu
*/
void Subscreen_Options()
{
    Subscreen_Internal_InitState();

    if (!subscreen_rendered)
    {
        subscreen_selection = 0;
        subscreen_audio_test_timer = 0;
        Subscreen_Internal_FindBottomEntry((const struct menu_item *)&subscreen_items_options);
        
        UserInterface_ClearWindowBuffer(false);
        UserInterface_ClearTextBuffer();

        UserInterface_DrawWindowBackground(0,0,32,3);
        UserInterface_DrawWindowBackground(0,3,32,25);

        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_HEADING, 3, 1);

        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_SOUND_MODE, 3, 4);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_SOUND_MODE_STEREO, 18, 5);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_SOUND_MODE_MONO, 27, 5);

        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_SOUND_MVOL, 3, 6);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_SOUND_BGMVOL, 3, 8);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_SOUND_SFXVOL, 3, 10);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_SOUND_VOIVOL, 3, 12);

        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_GFX_HITBLUR, 3, 14);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_ON, 20, 15);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_OFF, 27, 15);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_GFX_HEATWAVE, 3, 16);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_ON, 20, 17);
        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_OFF, 27, 17);

        UserInterface_DrawWindowText((char *)&STR_UI_SUBSCREEN_OPTIONS_RETURN, 3, 26);

        Subscreen_Options_DrawValues(false);
        
        UserInterface_CopyUiBuffers();
        
        subscreen_rendered = 1;
    }
    else
    {
        // Perform menu navigation
        bool update_text = false;
        bool volume_button_newly_pressed = false;
        bool volume_repeat_due = false;

        Subscreen_Internal_UpdateNavigation((const struct menu_item *)&subscreen_items_options);

        bool left_pressed = System_CheckKey(KEY_LEFT);
        bool right_pressed = System_CheckKey(KEY_RIGHT);
        bool toggle_pressed = left_pressed || right_pressed || System_CheckKey(KEY_A);

        if ((subscreen_selection >= 1 && subscreen_selection <= 4) && (System_CheckKeyHeld(KEY_LEFT) || System_CheckKeyHeld(KEY_RIGHT)))
        {
            volume_button_newly_pressed = left_pressed || right_pressed;

            if (volume_button_newly_pressed)
            {
                subscreen_options_volume_repeat_timer = 16;
                subscreen_options_volume_repeat_frame = system_frames_elapsed;
            }
            else if (subscreen_options_volume_repeat_frame != system_frames_elapsed)
            {
                subscreen_options_volume_repeat_frame = system_frames_elapsed;
                if (subscreen_options_volume_repeat_timer > 0)
                {
                    subscreen_options_volume_repeat_timer--;
                }
                volume_repeat_due = subscreen_options_volume_repeat_timer == 0;
            }

            if (volume_button_newly_pressed || volume_repeat_due)
            {
                uint8_t * target_vol_ptr = 0;
                switch (subscreen_selection)
                {
                    case 1:
                        target_vol_ptr = &snd_settings_volume_master;
                        break;
                    case 2:
                        target_vol_ptr = &snd_settings_volume_bgm;
                        break;
                    case 3:
                        target_vol_ptr = &snd_settings_volume_sfx;
                        break;
                    case 4:
                        target_vol_ptr = &snd_settings_volume_voice;
                        break;
                    default:
                        break;
                }

                if (target_vol_ptr != 0)
                {
                    uint8_t old_vol = *target_vol_ptr;
                    uint8_t new_vol = old_vol;

                    if (System_CheckKeyHeld(KEY_LEFT))
                    {
                        if (new_vol == 127)
                        {
                            new_vol = 120;
                        }
                        else if (new_vol >= 8)
                        {
                            new_vol -= 8;
                        }
                        else
                        {
                            new_vol = 0;
                        }
                    }
                    else if (System_CheckKeyHeld(KEY_RIGHT))
                    {
                        if (new_vol <= 119)
                        {
                            new_vol += 8;
                        }
                        else
                        {
                            new_vol = 127;
                        }
                    }

                    if (new_vol != old_vol)
                    {
                        *target_vol_ptr = new_vol;
                        switch (subscreen_selection)
                        {
                            case 1:
                                SoundInterface_SetMasterVolume(new_vol);
                                SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                                break;
                            case 2:
                                SoundInterface_SetMusicVolume(new_vol);
                                if (old_vol == 0 && new_vol > 0)
                                {
                                    SoundInterface_PlayMusic();
                                }
                                else if (old_vol > 0 && new_vol == 0)
                                {
                                    SoundInterface_PauseMusic();
                                }
                                break;
                            case 3:
                                SoundInterface_SetSfxVolume(new_vol);
                                SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                                break;
                            case 4:
                                SoundInterface_SetVoiceVolume(new_vol);
                                if (new_vol > 0)
                                {
                                    SoundInterface_PlayClip(STREAM_VOICE_HURT_1 + (Math_GetRandom_u16() & 0x01));
                                }
                                else
                                {
                                    SoundInterface_StopStream();
                                }
                                break;
                            default:
                                break;
                        }
                        update_text = true;
                    }
                }

                if (volume_repeat_due)
                {
                    subscreen_options_volume_repeat_timer = 4;
                }
            }
        }
        else if (toggle_pressed)
        {
            switch (subscreen_selection)
            {
                case 0:
                    snd_settings_mono = !snd_settings_mono;
                    SoundInterface_SetOutputMode(snd_settings_mono ? 1 : 0);
                    subscreen_audio_test_timer = 1;
                    update_text = true;
                    break;
                case 5:
                    gfx_enable_hitblur = !gfx_enable_hitblur;
                    SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                    update_text = true;
                    break;
                case 6:
                    gfx_enable_heatwave = !gfx_enable_heatwave;
                    SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
                    update_text = true;
                    break;
                default:
                    break;
            }
        }

        if (subscreen_audio_test_timer > 0)
        {
            struct game_object dummy_obj;
            dummy_obj.pos.x.a = 0;
            dummy_obj.pos.y.a = 0;
            dummy_obj.w = 0;
            dummy_obj.h = 0;

            if (subscreen_audio_test_timer == 1)
            {
                dummy_obj.pos.x.lh.h = bg_scroll_x.full.high.a + 1;
                SoundInterface_PlaySfx_Pre(&dummy_obj, SFX_ATK_PUNCH);
            }
            else if (subscreen_audio_test_timer == 14)
            {
                dummy_obj.pos.x.lh.h = bg_scroll_x.full.high.a + 128;
                SoundInterface_PlaySfx_Pre(&dummy_obj, SFX_ATK_PUNCH);
            }
            else if (subscreen_audio_test_timer == 27)
            {
                dummy_obj.pos.x.lh.h = bg_scroll_x.full.high.a + 255;
                SoundInterface_PlaySfx_Pre(&dummy_obj, SFX_ATK_PUNCH);
            }

            subscreen_audio_test_timer++;
            if (subscreen_audio_test_timer > 28)
            {
                subscreen_audio_test_timer = 0;
            }
        }

        if (update_text)
        {
            Subscreen_Options_DrawValues(true);
        }

        SpriteEngine_ProcessSpriteLists();

        uint16_t action_res = Subscreen_Internal_HandleMenuAction((const struct menu_item *)&subscreen_items_options);
        if (action_res == SUBSCREEN_ACTIONSTATE_RETURN)
        {
            return;
        }

        bool temp_exit_subscreen = (action_res == SUBSCREEN_ACTIONSTATE_EXIT);

        if (System_CheckKey(KEY_B) || temp_exit_subscreen)
        {
            Subscreen_Internal_GoBackToTop();
        }
    }

    return;
}

void Subscreen_Top_DrawTime_Internal_Format_6Chars(char *dest, uint16_t val)
{
    dest[0] = ' ';
    
    uint8_t d4 = 0;
    while (val >= 10000) {
        val -= 10000;
        d4++;
    }
    uint8_t d3 = 0;
    while (val >= 1000) {
        val -= 1000;
        d3++;
    }
    uint8_t d2 = 0;
    while (val >= 100) {
        val -= 100;
        d2++;
    }
    uint8_t d1 = 0;
    while (val >= 10) {
        val -= 10;
        d1++;
    }
    uint8_t d0 = (uint8_t)val;
    
    if (d4 != 0) {
        dest[1] = '0' + d4;
        dest[2] = '0' + d3;
        dest[3] = '0' + d2;
        dest[4] = '0' + d1;
        dest[5] = '0' + d0;
    } else if (d3 != 0) {
        dest[1] = ' ';
        dest[2] = '0' + d3;
        dest[3] = '0' + d2;
        dest[4] = '0' + d1;
        dest[5] = '0' + d0;
    } else if (d2 != 0) {
        dest[1] = ' ';
        dest[2] = ' ';
        dest[3] = '0' + d2;
        dest[4] = '0' + d1;
        dest[5] = '0' + d0;
    } else if (d1 != 0) {
        dest[1] = ' ';
        dest[2] = ' ';
        dest[3] = ' ';
        dest[4] = '0' + d1;
        dest[5] = '0' + d0;
    } else {
        dest[1] = ' ';
        dest[2] = ' ';
        dest[3] = ' ';
        dest[4] = ' ';
        dest[5] = '0' + d0;
    }
}

void Subscreen_Top_DrawTime_Internal_Format_2Chars_ZeroPadded(char *dest, uint16_t val)
{
    uint8_t d1 = 0;
    while (val >= 10) {
        val -= 10;
        d1++;
    }
    dest[0] = '0' + d1;
    dest[1] = '0' + (uint8_t)val;
}

void Subscreen_Top_DrawTime()
{
    char temp_time_string[48];

    uint16_t temp_h = system_time_h;
    uint16_t temp_m = system_time_m;
    uint16_t temp_s = system_time_s;

    // Dynamically copy the prefix from STR_UI_SUBSCREEN_PLAYTIME up to the first '%'
    uint16_t prefix_len = 0;
    while (STR_UI_SUBSCREEN_PLAYTIME[prefix_len] != '%' && STR_UI_SUBSCREEN_PLAYTIME[prefix_len] != '\0')
    {
        temp_time_string[prefix_len] = STR_UI_SUBSCREEN_PLAYTIME[prefix_len];
        prefix_len++;
    }

    Subscreen_Top_DrawTime_Internal_Format_6Chars(&temp_time_string[prefix_len], temp_h);

    char colon = (system_time_subframe >= (FPS / 2)) ? ' ' : ':';
    temp_time_string[prefix_len + 6] = colon;

    Subscreen_Top_DrawTime_Internal_Format_2Chars_ZeroPadded(&temp_time_string[prefix_len + 7], temp_m);

    temp_time_string[prefix_len + 9] = colon;

    Subscreen_Top_DrawTime_Internal_Format_2Chars_ZeroPadded(&temp_time_string[prefix_len + 10], temp_s);

    temp_time_string[prefix_len + 12] = '\0';

    UserInterface_DrawWindowText((char *)&temp_time_string, 7, 26);
    UserInterface_CopyTextBuffer_Line(26);

    return;
}

void Subscreen_ResetConfirmation()
{
    Subscreen_Internal_InitState();

    if (!subscreen_rendered)
    {
        subscreen_selection = 1;
        Subscreen_Internal_FindBottomEntry((const struct menu_item *)&subscreen_items_resetconfirm);
        
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
        Subscreen_Internal_UpdateNavigation((const struct menu_item *)&subscreen_items_resetconfirm);

        SpriteEngine_ProcessSpriteLists();

        uint16_t action_res = Subscreen_Internal_HandleMenuAction((const struct menu_item *)&subscreen_items_resetconfirm);
        if (action_res == SUBSCREEN_ACTIONSTATE_RETURN)
        {
            return;
        }

        bool temp_exit_subscreen = (action_res == SUBSCREEN_ACTIONSTATE_EXIT);

        if (System_CheckKey(KEY_B) || temp_exit_subscreen)
        {
            Subscreen_Internal_GoBackToTop();
        }
    }

    return;
}

void Subscreen_Internal_InitState()
{
    system_game_paused = true;
    system_dont_count_lag = true;

    hdma_use_gradient = 0xff;
    hdma_gradient_ptr = ADDR_LOWORD(&hdma_windowbackground_tables[1][0]);

    ui_in_subscreen = true;

    return;
}

void Subscreen_Internal_FindBottomEntry(const struct menu_item * item_array)
{
    subscreen_bottom_entry = 0;

    for (int i = 0; i < 256; i++)
    {
        if ((item_array[i].x == 255) && (item_array[i].y == 255))
        {
            subscreen_bottom_entry = i - 1;
            break;
        }
    }

    return;
}

uint16_t Subscreen_Internal_HandleMenuAction(const struct menu_item * item_array)
{
    if (System_CheckKey(KEY_A))
    {
        uint16_t action = item_array[subscreen_selection].action;
        void * ptr = item_array[subscreen_selection].ptr;

        if (action == MENUACTION_OPENSUBSCREEN)
        {
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
            if (ptr != 0)
            {
                Subscreen_Transition_Start(ptr);
                return SUBSCREEN_ACTIONSTATE_RETURN;
            }
        }
        else if (action == MENUACTION_OPENMAPSCREEN)
        {
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);

            shadow_brightness = 15 << 8;
            shadow_brightness_change = -(128 * V_MUL);

            system_use_alternate_nmi = true;

            system_loop_func_ptr = Main_GetFunctionPointer(ROUTINE_MAPDISPLAY_INIT);
            system_target_routine = ROUTINE_MAPDISPLAY_INIT;

            // Restore CGADSUB
            shadow_cgadsub = subscreen_cgadsub_copy;

            return SUBSCREEN_ACTIONSTATE_RETURN;
        }
        else if (action == MENUACTION_CALLFUNCTION)
        {
            SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
            if (ptr != 0)
            {
                void (*func)() = (void (*)())ptr;
                func();

                return SUBSCREEN_ACTIONSTATE_RETURN;
            }
        }
        else if (action == MENUACTION_EXITSUBSCREEN)
        {
            return SUBSCREEN_ACTIONSTATE_EXIT;
        }
    }

    return SUBSCREEN_ACTIONSTATE_NONE;
}

void Subscreen_Internal_GoBackToTop()
{
    SoundInterface_PlaySfx(SFX_UI_CONFIRM, 0);
    Subscreen_Transition_Start(Main_GetFunctionPointer(ROUTINE_SUBSCREEN));
    system_target_routine = ROUTINE_SUBSCREEN;

    return;
}

/*
    Called to update screen navigation
*/
void Subscreen_Internal_UpdateNavigation(const struct menu_item * item_array)
{
    if (System_CheckKey(KEY_UP))
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
    else if (System_CheckKey(KEY_DOWN))
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

    item_array += subscreen_selection;

    int16_t x = item_array->x;
    int16_t y = item_array->y;

    SpriteEngine_DrawUISprite(x, y, (0xa4 | PAL_SYS_CURSOR << 9 | 3 << 12));

    return;
}

const struct menu_item subscreen_items_toplevel[7] = {
    {6, 16, MENUACTION_EXITSUBSCREEN, 0}, 

    {6, 24, MENUACTION_OPENSUBSCREEN, (void *)&Subscreen_Upgrade}, 
    {6, 32, MENUACTION_OPENMAPSCREEN, 0}, 
    {6, 40, MENUACTION_OPENSUBSCREEN, (void *)&Subscreen_Help}, 
    {6, 48, MENUACTION_OPENSUBSCREEN, (void *)&Subscreen_Options}, 
    
    {6, 56, MENUACTION_OPENSUBSCREEN, (void *)&Subscreen_ResetConfirmation}, 

    {255, 255, 0, 0}, 
};

const struct menu_item subscreen_items_profile[5] = {
    {6, 152, MENUACTION_CALLFUNCTION, (void *)&Subscreen_Upgrade_Hp}, 
    {6, 168, MENUACTION_CALLFUNCTION, (void *)&Subscreen_Upgrade_Attack}, 
    {6, 184, MENUACTION_CALLFUNCTION, (void *)&Subscreen_Upgrade_Defense}, 

    {6, 208, MENUACTION_EXITSUBSCREEN, 0}, 

    {255, 255, 0, 0}, 
};

const struct menu_item subscreen_items_help[7] = {
    // These pointers are needed for the help system
    {-2, 32, 0, (void *)&STR_UI_SUBSCREEN_HELP_MOVEMENT}, 
    {-2, 48, 0, (void *)&STR_UI_SUBSCREEN_HELP_INTERACTION}, 
    {-2, 64, 0, (void *)&STR_UI_SUBSCREEN_HELP_ATTACK}, 
    {-2, 80, 0, (void *)&STR_UI_SUBSCREEN_HELP_PROGRESSION}, 
    {-2, 96, 0, (void *)&STR_UI_SUBSCREEN_HELP_MAP}, 
    {-2, 112, 0, (void *)&STR_UI_SUBSCREEN_HELP_RESET}, 
    
    {255, 255, 0, 0}, 
};

const struct menu_item subscreen_items_options[9] = {
    {6, 32, 0, 0}, 
    {6, 48, 0, 0}, 
    {6, 64, 0, 0}, 
    {6, 80, 0, 0}, 
    {6, 96, 0, 0}, 
    {6, 112, 0, 0}, 
    {6, 128, 0, 0}, 
    {6, 208, MENUACTION_EXITSUBSCREEN, 0}, 
    
    {255, 255, 0, 0}, 
};

const struct menu_item subscreen_items_resetconfirm[3] = {
    {6, 112, MENUACTION_CALLFUNCTION, (void *)&System_SoftReset}, 
    {6, 128, MENUACTION_EXITSUBSCREEN, 0}, 

    {255, 255, 0, 0}, 
};
