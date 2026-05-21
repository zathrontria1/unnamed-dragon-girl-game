#include <snes/console.h>

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "dma.h"

#include "ui.h"
#include "spr.h"

void UserInterface_Process()
{
    if ((ui_cached_hp != obj_player_pointer->struct_data.npc_data.hp) ||
        (ui_cached_hp_max != obj_player_pointer->struct_data.npc_data.hp_max) || (ui_force_update != 0))
    {
        // HP changed
        UserInterface_UpdateHealthCounters();
    }

    if ((ui_cached_money != obj_player_pointer->struct_data.npc_data.money) || (ui_force_update != 0))
    {
        // Amount of money held changed
        UserInterface_UpdateMoneyCounters();
    }

    if ((ui_cached_enemy_counter != obj_enemies_defeated) || (ui_force_update != 0))
    {
        // Enemies defeated changed
        UserInterface_UpdateEnemyCounters();
    }

    ui_force_update = 0;

    return;
}

void UserInterface_UpdateHealthCounters()
{
    // Copy these values
    int32_t temp_hp = obj_player_pointer->struct_data.npc_data.hp;
    int32_t temp_hp_max = obj_player_pointer->struct_data.npc_data.hp_max;

    // Calculate the amount of pixels the health bar would have
    uint16_t temp_bar_length_fill;
    uint16_t temp_bar_length_max;

    if (temp_hp > temp_hp_max)
    {
        temp_hp = temp_hp_max;
    }

    if (temp_hp_max < 208) // Less than 208 max HP
    {
        temp_bar_length_max = temp_hp_max;

        temp_bar_length_fill = temp_hp;
    }
    else // 208 or more max HP
    {
        temp_bar_length_max = 208; // limit to max 208 pixels

        // Scale down numbers larger than 8-bit
        while (temp_hp_max >= 0x10000)
        {
            temp_hp >>= 8;
            temp_hp_max >>= 8;
        }
        while (temp_hp_max >= 0x100)
        {
            temp_hp >>= 1;
            temp_hp_max >>= 1;
        }

        // Adjust the fill based on the fraction 
        temp_bar_length_fill = (uint16_t)(temp_hp * 208) / (uint8_t)temp_hp_max;
    }

    if (temp_hp <= 0)
    {
        temp_bar_length_fill = 0;
    }
    else if (temp_bar_length_fill == 0)
    {
        temp_bar_length_fill = 1;
    }
     
    uint16_t temp_bar_filled_tiles = temp_bar_length_fill >> 3; // div 8
    uint16_t temp_bar_partial_tile = temp_bar_length_fill - (temp_bar_filled_tiles << 3);
    uint16_t temp_bar_empty_tiles = (temp_bar_length_max >> 3) - temp_bar_filled_tiles;

    int i = 0;
    uint16_t temp_extra_length = 0;
    ui_hp_gauge[i] = 0x016b | 0x2000 | (PAL_UI_4BPP << 10);
    i++;

    for (; i < temp_bar_filled_tiles + 1; i++)
    {
        ui_hp_gauge[i] = 0x0168 | 0x2000 | (PAL_UI_4BPP << 10);
    }

    // Calculate the "behind" backing
    uint32_t temp_bar_tile_offset;

    switch ((temp_bar_length_max % 8) - 1)
    {
        case 0:
            temp_bar_tile_offset = 0;
            break;
        case 1:
            temp_bar_tile_offset = 32 * 3;
            break;
        case 2:
            temp_bar_tile_offset = 32 * 7;
            break;
        case 3:
            temp_bar_tile_offset = 32 * 12;
            break;
        case 4:
            temp_bar_tile_offset = 32 * 18;
            break;
        case 5:
            temp_bar_tile_offset = 32 * 25;
            break;
        case 6:
            temp_bar_tile_offset = 32 * 33;
            break;
        default: 
            temp_bar_tile_offset = 0;
    }

    if (temp_bar_partial_tile != 0)
    {
        if (i == (temp_bar_filled_tiles + temp_bar_empty_tiles + 1))
        {
            // Use the dynamic tile if the partial fill tile is 
            // the last tile
            ui_hp_gauge[i] = 0x0169 | 0x2000 | (PAL_UI_4BPP << 10);

            if ((temp_bar_length_max % 8) >= 5)
            {
                // One extra tile
                temp_extra_length = 1;
                ui_hp_gauge[i+1] = 0x016a | 0x2000 | (PAL_UI_4BPP << 10);

                if ((dma_queue_add(
                    (uint8_t *)(&data_ui_dynamic_hp) + (((temp_bar_length_max % 8) + 1) << 5) + temp_bar_tile_offset, 
                    0x56a0, 
                    32,
                    VRAM_INCHIGH, 
                    0 
                    ) != 0 ))
                {
                    // If we can't update the tile, just return (consider it failed)
                    return;
                }
            }

            if ((dma_queue_add(
                (uint8_t *)(&data_ui_dynamic_hp) + (temp_bar_partial_tile << 5) + temp_bar_tile_offset, 
                0x5690, 
                32,
                VRAM_INCHIGH, 
                0 
                ) != 0 ))
            {
                // If we can't update the tile, just return (consider it failed)
                return;
            }
        }
        else
        {
            ui_hp_gauge[i] = (0x0160 + temp_bar_partial_tile) | 0x2000 | (PAL_UI_4BPP << 10);
        }

        i++;
    }

    if ((temp_bar_length_max % 8) == 0)
    {
        temp_bar_empty_tiles--;
    }

    for (; i < (temp_bar_filled_tiles + temp_bar_empty_tiles + 2); i++)
    {
        if (i == (temp_bar_filled_tiles + temp_bar_empty_tiles + 1) && ((temp_bar_length_max % 8) != 00))
        {
            // last tile is partial
            // Use the dynamic tile if the partial fill tile is 
            // the last tile
            temp_extra_length = 1;
            ui_hp_gauge[i] = (0x0169) | 0x2000 | (PAL_UI_4BPP << 10);
            ui_hp_gauge[i+1] = (0x016a) | 0x2000 | (PAL_UI_4BPP << 10);

            if (dma_queue_add(
                    (uint8_t *)(&data_ui_dynamic_hp) + (((temp_bar_length_max % 8) + 1) << 5) + temp_bar_tile_offset, 
                    0x56a0, 
                    32,
                    VRAM_INCHIGH, 
                    0
                    ) != 0 )
            {
                // If we can't update the tile, just return (consider it failed)
                return;
            }

            if (dma_queue_add(
                (uint8_t *)(&data_ui_dynamic_hp) + temp_bar_tile_offset, 
                0x5690, 
                32,
                VRAM_INCHIGH, 
                0
                ) != 0)
            {
                // If we can't update the tile, just return (consider it failed)
                return;
            }
        }
        else
        {
            ui_hp_gauge[i] = 0x0160 | 0x2000 | (PAL_UI_4BPP << 10);
        }
    }

    if ((temp_bar_length_max % 8) == 0)
    {
        // One extra tile and the last tile was a full tile
        temp_extra_length = 1;
        ui_hp_gauge[i] = (0x016c) | 0x2000 | (PAL_UI_4BPP << 10);
    }

    // Upload the tilemap data itself
    if (dma_queue_add(
        (uint8_t *)(&ui_hp_gauge[0]), 
        0x3000 + 4 + ((UI_MARGIN_TOP) << 5), 
        (temp_bar_filled_tiles + temp_bar_empty_tiles + 2 + temp_extra_length) << 1,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        ui_cached_hp = temp_hp;
        ui_cached_hp_max = temp_hp_max;
    }

    return;
}

/*
    Set up the drawing for the HP gauge
*/
void UserInterface_DrawEnemyHealthBar(struct game_object * o)
{
    // get fraction of health if it's not changed
    if (o->struct_data.npc_data.hp_cache != o->struct_data.npc_data.hp)
    {
        uint32_t temp_hp = o->struct_data.npc_data.hp;
        uint32_t temp_hp_max = o->struct_data.npc_data.hp_max;

        while (temp_hp_max >= 0x10000)
        {
            temp_hp >>= 8;
            temp_hp_max >>= 8;
        }

        while (temp_hp_max >= 0x100)
        {
            temp_hp >>= 1;
            temp_hp_max >>= 1;
        }
        uint16_t temp_fraction = (uint16_t)(temp_hp * 10) / (uint8_t)temp_hp_max;

        if (temp_fraction == 0)
        {
            temp_fraction = 1;
        }
        else if (temp_fraction > 10)
        {
            temp_fraction = 10;
        }

        if (temp_fraction > 4)
        {
            o->struct_data.npc_data.hp_tile_offset = 96 + ((temp_fraction - 5) << 1);
        }
        else
        {
            o->struct_data.npc_data.hp_tile_offset = 64 + ((temp_fraction - 1) << 1);
        }

        o->struct_data.npc_data.hp_cache = o->struct_data.npc_data.hp;
    }

    SpriteEngine_DrawUISprite(
        o->pos.x.lh.h - bg_scroll_x.full.high.a, 
        o->pos.y.lh.h - bg_scroll_y.full.high.a -8, 
        (o->struct_data.npc_data.hp_tile_offset | PAL_SYS_IMPACT << 9 | 3 << 12));

    if (!system_game_paused)
    {
        o->struct_data.npc_data.hp_display_time--;
    }

    return;
}

void UserInterface_UpdateMoneyCounters()
{
    // Copy these values
    uint32_t temp_money = obj_player_pointer->struct_data.npc_data.money;

    uint8_t temp_string[6] = "     " ; // 5 spaces

    uint32_t temp_money_copy;

    // Reduce the visible length of the money.
    // Every digit is a significant cost in displaying the number.
    if (temp_money >= 10000000)
    {
        // at least 10000K
        temp_money_copy = temp_money / 1000000;
        temp_string[4] = 'M';
    }
    else if (temp_money >= 10000)
    {
        // at least 10000, exceeding 4 digits counter.
        temp_money_copy = temp_money / 1000;
        temp_string[4] = 'K';
    }
    else
    {
        // less than 10K, can be displayed fully
        temp_money_copy = temp_money;
    }

    // The icon
    ui_money_counter[0] = 0x016d | 0x2000 | (PAL_UI_4BPP << 10);

    // Now to make the counter digits itself.
    // 4 digits with denominator at a fixed location, so 6 characters
    
    //uint8_t * t = (uint8_t *)&temp_string; 

    //sprintf(t, "%10lu", temp_money); // Slower when dealing with just a number.

    uint16_t temp_len = 1;

    if (temp_money_copy != 0)
    {
        // Non-zero value
        for (int i = 3; i >= 0; i--)
        {
            temp_string[i] = 0x30 + (temp_money_copy % 10);
            temp_money_copy /= 10;

            if (temp_money_copy == 0)
            {
                break;
            }
        }
    }
    else
    {
        temp_string[3] = '0';
    }

    for (int i = 1; i < 6; i++)
    {
        ui_money_counter[i] = 0x00e0+(temp_string[i-1]) | 0x2000 | (PAL_UI_4BPP << 10);
    }

    if (dma_queue_add(
        (uint8_t *)(&ui_money_counter[0]), 
        0x3000 + 1 + ((27 - UI_MARGIN_TOP) << 5), 
        12,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        ui_cached_money = temp_money;
    }

    return;
}


void UserInterface_UpdateEnemyCounters()
{
    // Copy these values
    char temp_string[8] = "   /   "; // 3 spaces, a /, and another 3 spaces

    // The icon
    ui_enemy_counter[0] = 0x016e | 0x2000 | (PAL_UI_4BPP << 10);

    snprintf((char *)&temp_string, 8, "%3u/%3u", obj_enemies_defeated, obj_enemies_max_count);

    for (int i = 0; i < 8; i++)
    {
        ui_enemy_counter[i+1] = (0x00e0 + temp_string[i]) | 0x2000 | (PAL_UI_4BPP << 10);
    }

    //if (obj_enemies_defeated == obj_enemies_max_count)
    if (obj_enemies_defeated >= obj_enemies_target_count)
    {
        for (int i = 0; i < 5; i++)
        {
            ui_level_status[i] = (0x0174+i) | 0x2000 | (PAL_UI_4BPP << 10);
        }
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            ui_level_status[i] = (0x0100) | 0x2000 | (PAL_UI_4BPP << 10);
        }
    }

    if (dma_queue_add(
        (uint8_t *)(&ui_level_status[0]), 
        0x3000 + 0x12 + ((27 - UI_MARGIN_TOP) << 5),  
        10,
        VRAM_INCHIGH, 
        0
        ) != 0)
    {
        return;
    }

    if (dma_queue_add(
        (uint8_t *)(&ui_enemy_counter[0]), 
        0x3000 + 0x17 + ((27 - UI_MARGIN_TOP) << 5),  
        16,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        ui_cached_enemy_counter = obj_enemies_defeated;
    }

    return;
}

void UserInterface_PrintSpecialText(uint8_t * string_ptr)
{
    char temp_str[128];

    /*
        const uint8_t STR_UI_PLAYERINFO_ML[] = "\
        HP: %u/%u\n\
        ATK: %u DEF: %u\n\
        Lag frames:         %10u\n\
        Play time:        %6u:%02u:%02u";
    */

    uint16_t temp_hp = obj_player_pointer->struct_data.npc_data.hp;
    uint16_t temp_hp_max = obj_player_pointer->struct_data.npc_data.hp_max;
    uint16_t temp_attack = obj_player_pointer->struct_data.npc_data.attack;
    uint16_t temp_defense = obj_player_pointer->struct_data.npc_data.defense;
    uint16_t temp_lagframes = system_frames_lag;
    uint16_t temp_h = (uint16_t)((system_frames_elapsed / FPS) / (3600l));
    uint16_t temp_m = (uint16_t)(((system_frames_elapsed / FPS) % (3600l)) / 60);
    uint16_t temp_s = (uint16_t)((system_frames_elapsed / FPS) % 60);

    snprintf(
        (char *)&temp_str, 128, (char *)string_ptr, 
        temp_hp, temp_hp_max, temp_attack, temp_defense, temp_lagframes, temp_h, temp_m, temp_s);
    
    UserInterface_PrintText_MultiLine((uint8_t *)&temp_str, UI_MSGBOX_ML_START, UI_MARGIN_LEFT);

    return;
}

/* 
    Prints 4-line monospaced text to the screen and queues DMA for it
*/
void UserInterface_PrintText_MultiLine(uint8_t * string_ptr, uint16_t row, uint16_t col)
{
    uint16_t temp_col = 0;
    uint16_t temp_row = 0;

    if (ui_show_message_page == 0)
    {
        // Start of a new message page.
        ui_show_message_page_ptr_init = string_ptr;
        UserInterface_DrawTextbox(UI_MSGBOX_ML_START, UI_MSGBOX_HEIGHT);
    }
    else if (string_ptr == ui_show_message_page_ptr_init)
    {
        // Continuing from existing page of messages
        string_ptr = ui_show_message_page_ptr;
    }
    else
    {
        // New message, force reset
        ui_show_message_page_ptr_init = string_ptr;
        ui_show_message_page = 0;
    }

    for (; temp_row < 4;)
    {
        if (*string_ptr == 0x00)
        {
            int i = temp_col;
            // Null terminator.
            ui_show_message_page = 0;

            // Fill the rest of the text buffer with spaces
            for (int j = temp_row; j < 4; j++)
            {
                for (; i < 30; i++)
                {
                    ui_window_text[j][i] = 0x0000 | 0x2000;
                }

                i = 0;
            }
            
            break;
        }
        else if (*string_ptr == '\r')
        {
            int i = temp_col;
            // Page break.
            ui_show_message_page++;

            // Fill the rest of the text buffer with spaces
            for (int j = temp_row; j < 4; j++)
            {
                for (; i < 30; i++)
                {
                    ui_window_text[j][i] = 0x0000 | 0x2000 | (PAL_UI_TEXT_WHITE << 10);
                }

                i = 0;
            }
            string_ptr++;
            break;
        }
        else if (*string_ptr == '\n')
        {
            // Fill the rest of the line with spaces, then
            // advance the printer to the next line.
            for (int i = temp_col; temp_col < 30; temp_col++)
            {
                ui_window_text[temp_row][temp_col] = 0x0000 | 0x2000 | (PAL_UI_TEXT_WHITE << 10);
            }
            temp_col = 0;
            temp_row++;
            string_ptr++;
            continue;
        }
        else if (temp_col >= 30)
        {
            // Exceeds current line bounds.
            // Word wrap is not implemented, so words will be torn apart!
            temp_col = 0;
            temp_row++; // This might be followed by...
        }

        // Test this right before printing.
        if (temp_row >= 4)
        {
            // Incorrectly formatted text - row overflow
            // Assume that the string has ended at this point.
            ui_show_message_page = 0;
            break;
        }

        // Normal printable.
        ui_window_text[temp_row][temp_col] = (-0x0020 + *string_ptr) | 0x2000 | (PAL_UI_TEXT_WHITE << 10);


        temp_col++;
        string_ptr++;
    }

    ui_show_message_page_ptr = string_ptr;

    if (dma_queue_add(
        (uint8_t *)(&ui_window_text[0][0]), 
        0x3400 + ((row + 1) << 5) + (col), 
        252,
        VRAM_INCHIGH, 
        0
        ) == 1)
        {
            return;
        }

    if (dma_queue_add(
        (uint8_t *)(&const_ui_textadvance_tilemapentries[0]), 
        0x3400 + ((row + 5) << 5) + (31 - col), 
        4,
        VRAM_INCHIGH, 
        0
        ) == 1)
        {
            return;
        }

    if (dma_queue_add(
        (uint8_t *)(&const_ui_textadvance_tilemapentries[2]), 
        0x3400 + ((row + 6) << 5) + (31 - col), 
        4,
        VRAM_INCHIGH, 
        0
        ) == 1)
        {
            return;
        }

    ui_show_message_cleared = 0;
    
    return;
}

/*
    Prints monospaced text to the screen and queues DMA for it
*/
void UserInterface_PrintText(uint8_t * string_ptr, uint16_t row, uint16_t col)
{
    int i = 0;
    uint16_t temp_len = 2;
    for (; i < 30; i++)
    {
        ui_window_text[0][i] = (-0x20 + *string_ptr++) | 0x2000  | (PAL_UI_TEXT_WHITE << 10);

        if ((*string_ptr == 0x00) || (*string_ptr == '\n') || (*string_ptr == '\r'))
        {
            i++;
            break;
        }
    }

    if (i != 0)
    {
        temp_len = i << 1;
    }

    for (; i < 30; i++)
    {
        ui_window_text[0][i] = 0x0000 | 0x2000;
    }

    if (dma_queue_add(
        (uint8_t *)(&ui_window_text[0][0]), 
        0x3400 + (row << 5) + (col), 
        temp_len,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        ui_show_message_cleared = 0;
        ui_show_message_ttl = 60 / V_MUL;
    }
    return;
}

/*
    Prints monospaced text to the screen and queues DMA for it, for mode 3 
    (where 4bpp and UI tilemap is located in a different location)
*/
void UserInterface_PrintText_Mode3(uint8_t * string_ptr, uint16_t row, uint16_t col)
{
    uint16_t i = 0;
    uint16_t temp_len = 2;
    for (; i < 30; i++)
    {
        ui_window_text[0][i] = (0x00e0 + *string_ptr++) | 0x2000 | (PAL_UI_4BPP << 10);

        if ((*string_ptr == 0x00) || (*string_ptr == '\n') || (*string_ptr == '\r'))
        {
            i++;
            break;
        }
    }

    if (i != 0)
    {
        temp_len = i << 1;
    }

    for (; i < 30; i++)
    {
        ui_window_text[0][i] = 0x0100 | 0x2000;
    }

    if (dma_queue_add(
        (uint8_t *)(&ui_window_text[0][0]), 
        0x4c00 + (row << 5) + (col), 
        temp_len,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        ui_show_message_cleared = 0;
        ui_show_message_ttl = 60 / V_MUL;
    }
    return;
}

void UserInterface_ClearText(uint16_t len, uint16_t row, uint16_t col)
{
    for (int i = 0; i < len; i++)
    {
        ui_window_text[0][i] = 0x0000 | 0x2000 | (PAL_UI_TEXT_WHITE << 10);
    }
    
    if (dma_queue_add(
        (uint8_t *)(&ui_window_text[0][0]), 
        0x3400 + (row << 5) + (col), 
        len << 1,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        ui_show_message_cleared = 1;
    }

    return;
}

/* 
    Textbox border drawing and clearing functions

    Note that they always take the full width of the screen.

    Height includes borders (e.g. 4 text rows = h is 6)
*/
void UserInterface_DrawTextbox(uint16_t row, uint16_t h)
{
    ui_window_background[0][0] = 0x0170 | 0x2000 | (PAL_UI_4BPP << 10);
    ui_window_background[1][0] = 0x0180 | 0x2000 | (PAL_UI_4BPP << 10);

    ui_window_background[2][0] = 0x0190 | 0x2000 | (PAL_UI_4BPP << 10);
    ui_window_background[3][0] = 0x0180 | 0x2000 | (PAL_UI_4BPP << 10);

    ui_window_background[4][0] = 0x0190 | 0x2000 | (PAL_UI_4BPP << 10);
    ui_window_background[5][0] = 0x01a0 | 0x2000 | (PAL_UI_4BPP << 10);

    for (int i = 0; i < 30; i++)
    {
        ui_window_background[0][i+1] = (0x0171 + (i % 2)) | 0x2000 | (PAL_UI_4BPP << 10);
        ui_window_background[1][i+1] = (0x0181 + (i % 2))  | 0x2000 | (PAL_UI_4BPP << 10);

        ui_window_background[2][i+1] = (0x0191 + (i % 2)) | 0x2000 | (PAL_UI_4BPP << 10);
        ui_window_background[3][i+1] = (0x0181 + (i % 2)) | 0x2000 | (PAL_UI_4BPP << 10);

        ui_window_background[4][i+1] = (0x0191 + (i % 2)) | 0x2000 | (PAL_UI_4BPP << 10);
        ui_window_background[5][i+1] = (0x01a1 + (i % 2))  | 0x2000 | (PAL_UI_4BPP << 10);
    }

    ui_window_background[0][31] = 0x0173 | 0x2000 | (PAL_UI_4BPP << 10);
    ui_window_background[1][31] = 0x0183 | 0x2000 | (PAL_UI_4BPP << 10);

    ui_window_background[2][31] = 0x0193 | 0x2000 | (PAL_UI_4BPP << 10);
    ui_window_background[3][31] = 0x0183 | 0x2000 | (PAL_UI_4BPP << 10);

    ui_window_background[4][31] = 0x0193 | 0x2000 | (PAL_UI_4BPP << 10);
    ui_window_background[5][31] = 0x01a3 | 0x2000 | (PAL_UI_4BPP << 10);

    UserInterface_CopyTextboxToVram(row, h);

    return;
}

/*
    These functions clear the entirety of BG1 and BG3 UI buffers.
    Always call these before drawing any genericized UI windows or text
*/
void UserInterface_ClearWindowBuffer()
{
    for (int x = 0; x < 32; x++)
    {
        for (int y = 0; y < 32; y++)
        {
            ui_window_background[y][x] = 0x0100 | 0x2000 | (PAL_UI_4BPP << 10);
        }
    }
    return;
}
void UserInterface_ClearTextBuffer()
{
    for (int x = 0; x < 32; x++)
    {
        for (int y = 0; y < 32; y++)
        {
            ui_window_text[y][x] = 0x0000 | 0x2000 | (PAL_UI_TEXT_WHITE << 10);
        }
    }
    return;
}

/*
    This function can draw a window anywhere, but if starting from scratch, must flush the old background

    Right now it doesn't handle window sizes that aren't a multiple of 2 properly due to lack of suitable tiles
*/
void UserInterface_DrawWindowBackground(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    // Sanity checks
    if ((w == 0) || (h == 0))
    {
        // Size is 0
        return;
    }
    if ((x >= 32) || (y >= 32))
    {
        // Start of window is outside the screen
        return;
    }

    // First column
    for (int i = y; i < y+h; i++)
    {
        if (i >= 32)
        {
            // Over last row
            break;
        }

        if (i - y == 0)
        {
            // First row
            ui_window_background[i][x] = 0x0170 | 0x2000 | (PAL_UI_4BPP << 10);
        }
        else if ((i - y == 1) && (i != (y + h - 1)))
        {
            // Second row, and not the last row
            ui_window_background[i][x] = 0x0180 | 0x2000 | (PAL_UI_4BPP << 10);
        }
        else if (i == (y + h - 2))
        {
            // Second to last row
            ui_window_background[i][x] = 0x0190 | 0x2000 | (PAL_UI_4BPP << 10);
        }
        else if (i == (y + h - 1))
        {
            // Last row
            ui_window_background[i][x] = 0x01a0 | 0x2000 | (PAL_UI_4BPP << 10);
        }
        else if ((i - y) & 0x0001 == 1)
        {
            // Odd row
            ui_window_background[i][x] = 0x0180 | 0x2000 | (PAL_UI_4BPP << 10);
        }
        else
        {
            // Even row
            ui_window_background[i][x] = 0x0190 | 0x2000 | (PAL_UI_4BPP << 10);
        }
    }

    // Middle columns
    if (w-2 > 0)
    {
        for (int i = y; i < y+h; i++)
        {
            if (i >= 32)
            {
                // Over last row
                break;
            }

            for (int j = x+1; j < x+w-1; j++)
            {
                if (j >= 32)
                {
                    // Over last column
                    break;
                }

                if (i - y == 0)
                {
                    // First row
                    if (x % 2 == 0)
                    {
                        ui_window_background[i][j] = (0x0171 + ((j + 1) % 2)) | 0x2000 | (PAL_UI_4BPP << 10);
                    }
                    else
                    {
                        ui_window_background[i][j] = (0x0171 + (j % 2)) | 0x2000 | (PAL_UI_4BPP << 10);
                    }
                }
                else if (i == (y + h - 1))
                {
                    // Last row
                    if (x % 2 == 0)
                    {
                        ui_window_background[i][j] = (0x01a1 + ((j + 1) % 2))  | 0x2000 | (PAL_UI_4BPP << 10);
                    }
                    else
                    {
                        ui_window_background[i][j] = (0x01a1 + (j % 2))  | 0x2000 | (PAL_UI_4BPP << 10);
                    }
                }
                else if ((i - y) & 0x0001 == 1)
                {
                    // Odd row
                    if (x % 2 == 0)
                    {
                        ui_window_background[i][j] = (0x0181 + ((j + 1) % 2)) | 0x2000 | (PAL_UI_4BPP << 10);
                    }
                    else
                    {
                        ui_window_background[i][j] = (0x0181 + (j % 2)) | 0x2000 | (PAL_UI_4BPP << 10);
                    }
                }
                else
                {
                    // Even row
                    if (x % 2 == 0)
                    {
                        ui_window_background[i][j] = (0x0191 + ((j + 1) % 2)) | 0x2000 | (PAL_UI_4BPP << 10);
                    }
                    else
                    {
                        ui_window_background[i][j] = (0x0191 + (j % 2)) | 0x2000 | (PAL_UI_4BPP << 10);
                    }
                }
            }
        }
    }

    // Rightmost column
    for (int i = y; i < y+h; i++)
    {
        if (i >= 32)
        {
            // Over last row
            break;
        }

        if (i - y == 0)
        {
            // First row
            ui_window_background[i][x+w-1] = 0x0173 | 0x2000 | (PAL_UI_4BPP << 10);
        }
        else if ((i - y == 1) && (i != (y + h - 1)))
        {
            // Second row, and not the last row
            ui_window_background[i][x+w-1] = 0x0183 | 0x2000 | (PAL_UI_4BPP << 10);
        }
        else if (i == (y + h - 2))
        {
            // Second to last row
            ui_window_background[i][x+w-1] = 0x0193 | 0x2000 | (PAL_UI_4BPP << 10);
        }
        else if (i == (y + h - 1))
        {
            // Last row
            ui_window_background[i][x+w-1] = 0x01a3 | 0x2000 | (PAL_UI_4BPP << 10);
        }
        else if ((i - y) & 0x0001 == 1)
        {
            // Odd row
            ui_window_background[i][x+w-1] = 0x0183 | 0x2000 | (PAL_UI_4BPP << 10);
        }
        else
        {
            // Even row
            ui_window_background[i][x+w-1] = 0x0193 | 0x2000 | (PAL_UI_4BPP << 10);
        }
    }

    // Last row
    return;
}


/*
    Call this to draw a string to the text buffer.

    Text will newline if it hits a screen edge, and terminate at the last block
*/
void UserInterface_DrawWindowText(char * string_ptr, uint16_t x, uint16_t y)
{
    // Sanity check
    if ((x >= 32) || (y >= 32))
    {
        // Start of text is offscreen
        return;
    }

    uint16_t col = x;
    uint16_t row = y;

    while (*string_ptr != 0x00)
    {
        if (*string_ptr == '\n')
        {
            row++;
            col = x;
            string_ptr++;
        }
        else if (col >= 32)
        {
            row++;
            col = x;
        }
        
        if (row >= 32)
        {
            break;
        }

        ui_window_text[row][col] = (-0x20 + *string_ptr++) | 0x2000  | (PAL_UI_TEXT_WHITE << 10);

        col++;
    }

    return;
}

/*
    Call this to copy the entire UI buffer.
*/
void UserInterface_CopyUiBuffers()
{
    if (dma_queue_add(
        (uint8_t *)(&ui_window_background[0][0]), 
        0x3000, 
        2048,
        VRAM_INCHIGH, 
        0
        ) != 0)
    {
        return;
    }

    if (dma_queue_add(
        (uint8_t *)(&ui_window_text[0][0]), 
        0x3400, 
        2048,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        return;
    }

    return;
}

void UserInterface_ClearTextbox(uint16_t row, uint16_t h)
{
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            ui_window_background[i][j] = 0x0100 | 0x2000 | (PAL_UI_4BPP << 10);
        }
    }

    UserInterface_CopyTextboxToVram(row, h);
    UserInterface_ClearTextboxText(row, h);

    return;
}

void UserInterface_ClearTextboxText(uint16_t row, uint16_t h)
{
    for (int i = 0; i < 32; i++)
    {
        ui_window_text[0][i] = 0x0000 | 0x2000;
    }

    for (int i = 1; i < (h + 1); i++)
    {
        if (dma_queue_add(
            (uint8_t *)(&ui_window_text[0][0]), 
            0x3400 + ((row + i) << 5), 
            64,
            VRAM_INCHIGH, 
            0
            ) != 0)
        {
            return;
        }
    }

    return;
}

void UserInterface_CopyTextboxToVram(uint16_t row, uint16_t h)
{
    if (dma_queue_add(
        (uint8_t *)(&ui_window_background[0][0]), 
        0x3000 + (row << 5), 
        128,
        VRAM_INCHIGH, 
        0
        ) != 0)
    {
        return;
    }

    for (int i = 2; i < (h - 1); i++)
    {
        if (dma_queue_add(
            (uint8_t *)(&ui_window_background[2+(i%2)][0]), 
            0x3000 + ((row + i) << 5), 
            64,
            VRAM_INCHIGH, 
            0
            ) != 0)
        {
            return;
        }
    }

    if (dma_queue_add(
        (uint8_t *)(&ui_window_background[4][0]), 
        0x3000 + ((row + h - 2) << 5), 
        128,
        VRAM_INCHIGH, 
        0
        ) != 0)
    {
        return;
    }

    return;
}

void UserInterface_CopyUiGraphicsToVram()
{
    dma_queue_add((uint8_t *)((uint32_t)&data_ui_dynamic_textadvance + ((uint16_t)((system_frames_elapsed >> 2) % 4) * 32)), 0x4300, 32, VRAM_INCHIGH, 0);
    dma_queue_add((uint8_t *)((uint32_t)&data_ui_dynamic_textadvance + 256 + ((uint16_t)((system_frames_elapsed >> 2) % 4) * 32)), 0x4380, 32, VRAM_INCHIGH, 0);
    
    return;
}