#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "level.h"

#include "ani.h"
#include "ani_bg.h"
#include "ani_fixedspr.h"
#include "ani_pal.h"
#include "ani_pal_hdma.h"

#include "dma.h"
#include "system.h"
#include "spr.h"
#include "loop.h"
#include "obj.h"
#include "map.h"
#include "gfx.h"

#include "ui.h"

#include "lz4.h"

#include "math_int.h"

#include "data_strings.h"

#include "snd.h"
#include "consts_snd.h"

#include "main.h"

void loop_fadein()
{
    // Always reset mosaic during fades
    shadow_mosaic = 0x00; 

    // Always reset colour math during fades
    shadow_cgwsub = 0x00;
    shadow_cgadsub = 0x00;
    shadow_coldata_r = 0x00;
    shadow_coldata_g = 0x00;
    shadow_coldata_b = 0x00;

    shadow_inidisp += 1;

    if (shadow_inidisp >= 15)
    {
        system_loop_func_ptr = main_GetFunctionPointer(system_target_routine);
        //system_current_routine = system_target_routine;
    }

    return;
}

/*
    This loop is only used by the soft reset on game over routine.
    It's very likely it'll be gone entirely once a Game Over screen is implemented.
*/
void loop_fadeout()
{
    // Always reset mosaic during fades
    shadow_mosaic = 0x00; 

    // Always reset colour math during fades
    shadow_cgwsub = 0x00;
    shadow_cgadsub = 0x00;
    shadow_coldata_r = 0x00;
    shadow_coldata_g = 0x00;
    shadow_coldata_b = 0x00;

    shadow_inidisp -= 1;

    if (shadow_inidisp == 0)
    {
        system_loop_func_ptr = main_GetFunctionPointer(system_target_routine);
        //system_current_routine = system_target_routine;
    }

    return;
}

void loop_messagebox()
{
    system_game_paused = 1;

    obj_run();

    ui_dma_ui_tiles();

    spr_queue_process();

    spr_reset_sprites();
    spr_pack_oam();

    if (system_check_for_key(KEY_A))
    {
        snd_play_sfx(SFX_UI_CONFIRM, 0);
        
        ui_show_message_ttl = 0;
        ui_show_message_cleared = 1;

        if (ui_show_message_page == 0)
        {
            // Clear the textbox
            ui_clear_textbox(UI_MSGBOX_ML_START, UI_MSGBOX_HEIGHT);

            if (ui_show_message_page_ptr_init == (uint8_t *)&STR_MSG_TUTORIAL_MP)
            {
                event_tutorial_shown = 1;
            }
            
            system_game_paused = 0;

            system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_GAMELOOP);
            //system_current_routine = ROUTINE_GAMELOOP;
            system_target_routine = ROUTINE_GAMELOOP;
        }
        else if (ui_show_message_page != 0)
        {
            ui_print_ml(ui_show_message_page_ptr_init, UI_MSGBOX_ML_START, UI_MARGIN_LEFT);
        }
    }

    return;
}

void loop_game()
{
    ui_process();

    if (!event_tutorial_shown)
    {
        snd_play_sfx(SFX_UI_CONFIRM, 0);

        ui_print_ml((uint8_t *)&STR_MSG_TUTORIAL_MP, UI_MSGBOX_ML_START, UI_MARGIN_LEFT);

        system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_MSGBOX);
        //system_current_routine = ROUTINE_MSGBOX;
        system_target_routine = ROUTINE_MSGBOX;

        return;
    }

    if (system_check_for_key(KEY_X))
    {
        snd_play_sfx(SFX_UI_CONFIRM, 0);
        
        ui_print_ml_special((uint8_t *)&STR_UI_PLAYERINFO_ML);

        system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_MSGBOX);
        //system_current_routine = ROUTINE_MSGBOX;
        system_target_routine = ROUTINE_MSGBOX;

        return;
    }

    if (ui_show_message_ttl != 0)
    {
        ui_show_message_ttl--;
    }

    if (ui_show_message_ttl == 0 && !ui_show_message_cleared)
    {
        ui_clear(30, UI_MSGBOX_SL_START, 1);
    }

    ani_bg_update_water_anim();
    ani_bg_update_bg_anim();
    ani_fixedspr_process();
    ani_pal_process();

    obj_run();

    spr_queue_process();
    spr_reset_sprites();
    spr_pack_oam();

    obj_cleanup();
    obj_cleanup_hitbox_player();

    // Don't bother checking for input if the player is dying
    if (objects[obj_player_index].state != STATE_DIE)
    {
        if (system_check_for_key(KEY_SELECT))
        {
            // Let's try using the new alternate NMI part
            shadow_inidisp = 0x0f;
            system_use_alternate_nmi = 1;
            shadow_inidisp_change = -1;

            system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_MAPDISPLAY_INIT);
            //system_current_routine = ROUTINE_MAPDISPLAY_INIT;
            system_target_routine = ROUTINE_MAPDISPLAY_INIT;
        }
        else if (system_check_for_key(KEY_START))
        {
            system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_PAUSE);
            //system_current_routine = ROUTINE_PAUSE;
            system_target_routine = ROUTINE_PAUSE;

            shadow_inidisp = 0x08;
        }
    }

    gfx_process_mosaic();
    gfx_process_screen_cmath();
    
    return;
}

void loop_pause()
{
    // Silence the looping fire sound
    if (snd_flame_playing == 1)
    {
        snd_stop_sfx(SFX_ATK_FIRE_BREATH);
        snd_flame_playing = 0;
    }
    
    if (system_check_for_key(KEY_START))
    {
        system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_GAMELOOP);
        //system_current_routine = ROUTINE_GAMELOOP;
        system_target_routine = ROUTINE_GAMELOOP;
        
        shadow_inidisp = 0x0f;
    }

    return;
}

void loop_mapdisplay_init()
{
    // Silence the looping fire sound
    if (snd_flame_playing == 1)
    {
        snd_stop_sfx(SFX_ATK_FIRE_BREATH);
        snd_flame_playing = 0;
    }

    bg_scroll_x_saved = bg_scroll_x;
    bg_scroll_y_saved = bg_scroll_y;

    bg_scroll_x.full.high.a = -32;
    bg_scroll_y.full.high.a = -8;

    // Copy the current OAM into the copy
    shadow_oam_copy = shadow_oam;

    spr_queue_process();
    spr_reset_sprites();
    spr_pack_oam();

    struct game_object * o = &objects[obj_player_index];
    struct game_object temp_icon_object = objects[obj_player_index];
    temp_icon_object.state = STATE_ICON_NORMAL;
    temp_icon_object.facing = FACING_DOWN;

    struct game_data_npc * d = (struct game_data_npc *)&temp_icon_object.struct_data;
    d->ani.frame = 0;

    obj_player_prev_sprframe = ani_getframe_player(o); 
    uint8_t * temp_addr = ani_getframe_player(&temp_icon_object);

    dma_queue_add(temp_addr, 0x6000, 128, VRAM_INCHIGH, 1);

    system_ui_in_bg2 = 1;

    LZ4_UnpackToWRAM(level_data_ptr->map_overview_tiles_lz4, 0x007f0000);

    while (shadow_inidisp != 0x00)
    {
        ; // Prevent execution from continuing to VRAM writing parts while the display is turned on
    }

    // Try to move as many things as possible before here.
    system_interrupt_disable();
    REG_INIDISP = 0x8f;

    REG_HDMAEN = 0x00;

    REG_BG2HOFS = 0;
    REG_BG2HOFS = 0;
    REG_BG2VOFS = 0xff;
    REG_BG2VOFS = 0xff;

    // Copy the ROM palette into shadow
    dma_copy_to_wram((unsigned long int)level_data_ptr->map_overview_palette, (unsigned long int)&shadow_cgram, 480);
    dma_copy_to_wram((unsigned long int)(level_data_ptr->tileset_palette)+256, (unsigned long int)(&shadow_cgram)+480, 32);
    
    // Copy the background graphics into VRAM
    dma_copy_to_vram(0x007f0000, 0x0000, 0x9000);

    int i = 0;
    REG_VMAIN = VRAM_INCHIGH;
    REG_VMADDLH = TILEMAP_ADDR_MAP_MAP;
    for (int j = 0; j < 1024; j++)
    {
        if (((j & 0x1f) >= 24) || (i >= 576))
        {
            REG_VMDATALH = 808; // the guaranteed empty tile in the OAM
        }
        else
        {
            REG_VMDATALH = i;
            i++;
        }
    }

    #if VBCC_ASM == 1
        REG_VMAIN = VRAM_INCLOW;
        REG_VMADDLH = TILEMAP_ADDR_MAP_UI;

        __asm(
            "\ta8\n"
            "\tsep #$20\n"

            "\tldx #256\n"
            "\tstx r0\n"

            "\tlda #$08\n"
            "\tsta $4300\n"
            
            "\tldx #<r0\n"
            "\tstx $4302\n"
            "\tlda #^r0\n"
            "\tsta $4304\n"

            "\tldx #1024\n"
            "\tstx $4305\n"

            "\tlda #$18\n"
            "\tsta $4301\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );

        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_MAP_UI;

        __asm(
            "\ta8\n"
            "\tsep #$20\n"

            "\tlda #$08\n"
            "\tsta $4300\n"
            
            "\tldx #<r0+1\n"
            "\tstx $4302\n"
            "\tlda #^r0\n"
            "\tsta $4304\n"

            "\tldx #1024\n"
            "\tstx $4305\n"

            "\tlda #$19\n"
            "\tsta $4301\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_MAP_UI;

        for (int l = 0; l < 1024; l++)
        {
            REG_VMDATALH = 256;
        }
    #endif

    ui_print_mode3((uint8_t *)&STR_UI_HELP_MAP, UI_MAPSCREEN_SL_START, UI_MARGIN_LEFT);

    system_target_routine = ROUTINE_MAPDISPLAY;

    system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_FADEIN);
    //system_current_routine = ROUTINE_FADEIN;
    
    system_setup_tilemap_display(system_target_routine);
    system_init_display(system_target_routine);

    shadow_inidisp = 0x00;

    system_use_alternate_nmi = 0;
    shadow_inidisp_change = 0;

    system_interrupt_enable();

    return;
}


void loop_mapdisplay()
{
    // The minimap is always 192x192 pixels.
    // minimap position = (real position / map extent) * 192

    struct game_object * p = &objects[obj_player_index];

    int16_t temp_x = p->pos.x.lh.h;
    int16_t temp_y = p->pos.y.lh.h;

    int16_t temp_extent_x = map_extent_x;
    int16_t temp_extent_y = map_extent_y;

    if (temp_extent_x > temp_extent_y)
    {
        while (temp_extent_x > 255)
        {
            // Shift until all values fit
            temp_x >>= 1;
            temp_y >>= 1;
            temp_extent_x >>= 1;
            temp_extent_y >>= 1;
        }

        temp_x = (temp_x * 192) / temp_extent_x;
        temp_y = (temp_y * 192) / temp_extent_x;
    }
    else
    {
        while (temp_extent_y > 255)
        {
            // Shift until all values fit
            temp_x >>= 1;
            temp_y >>= 1;
            temp_extent_x >>= 1;
            temp_extent_y >>= 1;
        }

        temp_x = ((temp_x * 192) / temp_extent_y);
        temp_y = ((temp_y * 192) / temp_extent_y);
    }

    // Note that the player palette is in a different location
    struct game_data_npc * d = (struct game_data_npc *)&(p->struct_data);
    uint16_t temp_tileattrib = (d->ani.display | 7 << 9 | 3 << 12);

    if (((uint16_t) system_frames_elapsed % (60 / V_MUL)) < (30 / V_MUL))
    {
        spr_queue_add_ui(temp_x - bg_scroll_x.full.high.a - 6,temp_y - bg_scroll_y.full.high.a - 6, temp_tileattrib);
    }

    spr_queue_process();

    spr_reset_sprites();
    spr_pack_oam();

    if (system_check_for_key(KEY_SELECT))
    {
        // Let's try using the new alternate NMI part
        shadow_inidisp = 0x0f;
        system_use_alternate_nmi = 1;
        shadow_inidisp_change = -1;

        system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_GAMELOOP_RELOAD);
        //system_current_routine = ROUTINE_GAMELOOP_RELOAD;
        system_target_routine = ROUTINE_GAMELOOP_RELOAD;
    }

    return;
}

/*
    Used when going from another mode to the game (e.g. map screens)
*/

void loop_game_reload()
{
    // Perform a partial load
    level_load_tileset(level_data_ptr);
    level_load_palette(level_data_ptr);

    system_ui_in_bg2 = 0;
    ui_force_update = 1;

    bg_scroll_x = bg_scroll_x_saved;
    bg_scroll_y = bg_scroll_y_saved;

    dma_queue_add(obj_player_prev_sprframe, 0x6000, 128, VRAM_INCHIGH, 1);

    while (shadow_inidisp != 0x00)
    {
        ; // Prevent execution from continuing to VRAM writing parts while the display is turned on
    }

    // Try to move as many things as possible before here.
    system_interrupt_disable();
    REG_INIDISP = 0x8f;

    system_reset_bg_scroll_regs();
    

    // Copy the old OAM into the current
    shadow_oam = shadow_oam_copy;

    // DMA just the background and BG3 tiles
    dma_copy_to_vram(((uint32_t)0x007f0000 | ((uint32_t)TILEDATA_ADDR_GAME_MAP << 1)), TILEDATA_ADDR_GAME_MAP, 24576);
    dma_copy_to_vram(((uint32_t)0x007f0000 | ((uint32_t)TILEDATA_ADDR_GAME_UI_2BPP << 1)), TILEDATA_ADDR_GAME_UI_2BPP, 8192);
    map_regenerate();
    system_reset_ui_tilemap();

    system_loop_func_ptr = main_GetFunctionPointer(ROUTINE_FADEIN);
    //system_current_routine = ROUTINE_FADEIN;
    system_target_routine = ROUTINE_GAMELOOP;
    

    system_setup_tilemap_display(system_target_routine);
    system_init_display(system_target_routine);

    loop_game_partial();

    ani_pal_hdma_enable(); 

    shadow_inidisp = 0x00;

    system_use_alternate_nmi = 0;
    shadow_inidisp_change = 0;

    system_interrupt_enable();

    return;
}

/*
    Run during initialization or scene reload to make sure everything that is supposed to be there, is
*/
void loop_game_partial(void)
{
    // Make a copy of the current paused state
    uint16_t temp_game_paused_copy = system_game_paused;

    system_game_paused = 1;
    ui_process();
    obj_run();

    spr_queue_process();
    spr_reset_sprites();
    spr_pack_oam();
    system_game_paused = temp_game_paused_copy;

    return;
}