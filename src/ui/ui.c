#include "snes/console.h"

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "map.h"

#include "obj.h"

#include "dma.h"
#include "system.h"

#include "ui.h"
#include "ui_messagebox.h"
#include "ui_vwf.h"
#include "spr.h"

bool ui_in_subscreen;
bool ui_in_bg2;

// UI cache invalidation stuff
bool ui_force_update;
int32_t ui_cached_hp;
int32_t ui_cached_hp_max;
uint32_t ui_cached_money;
uint16_t ui_cached_enemy_counter;

// TODO: Handle UI windows and texts generically
uint16_t ui_window_background[(SCREEN_HEIGHT >> 3)][32]; // BG1. Call functions to draw a window here.
uint16_t ui_window_text[(SCREEN_HEIGHT >> 3)][32]; // BG3. Call functions to draw text here.

// Sub-strings
uint16_t ui_hp_gauge[28];
uint16_t ui_money_counter[11];
uint16_t ui_level_status[5];
uint16_t ui_enemy_counter[9];

uint32_t ui_display_money;

// UI status and timers
uint16_t ui_show_message_ttl;
bool ui_show_message_cleared;

/**
 * @brief Main UI engine processing routine.
 * 
 * Ticks HUD counters and triggers DMA uploads when stats change.
 */
void UserInterface_Process()
{
    if (ui_force_update)
    {
        // Update is forced and just do everything
        UserInterface_UpdateHealthCounters();
        UserInterface_UpdateMoneyCounters();
        UserInterface_UpdateEnemyCounters();

        ui_force_update = false;
    }
    else
    {
        if ((ui_cached_hp != obj_player_pointer->struct_data.npc_data.hp) ||
        (ui_cached_hp_max != obj_player_pointer->struct_data.npc_data.hp_max))
        {
            // HP changed
            UserInterface_UpdateHealthCounters();
        }

        if (ui_cached_money != obj_player_pointer->struct_data.npc_data.money)
        {
            // Amount of money held changed
            UserInterface_UpdateMoneyCounters();
        }

        if (ui_cached_enemy_counter != obj_enemies_defeated)
        {
            // Enemies defeated changed
            UserInterface_UpdateEnemyCounters();
        }
    }

    return;
}

/**
 * @brief Redraws the player HP gauge bar into the UI tilemap buffer.
 */
void UserInterface_UpdateHealthCounters()
{
    static const uint16_t hp_tile_offsets[8] = {
        0,         // & 7 == 0
        0,         // & 7 == 1
        32 * 3,    // & 7 == 2
        32 * 7,    // & 7 == 3
        32 * 12,   // & 7 == 4
        32 * 18,   // & 7 == 5
        32 * 25,   // & 7 == 6
        32 * 33    // & 7 == 7
    };

    // Copy raw values to prevent mutation from breaking 1 HP guard or UI cache
    int32_t raw_hp = obj_player_pointer->struct_data.npc_data.hp;
    int32_t raw_hp_max = obj_player_pointer->struct_data.npc_data.hp_max;

    if (raw_hp > raw_hp_max)
    {
        raw_hp = raw_hp_max;
    }

    int32_t scale_hp = raw_hp;
    int32_t scale_hp_max = raw_hp_max;

    // Calculate the amount of pixels the health bar would have
    uint16_t temp_bar_length_fill;
    uint16_t temp_bar_length_max;

    if (scale_hp_max < 208) // Less than 208 max HP
    {
        temp_bar_length_max = (uint16_t)scale_hp_max;
        temp_bar_length_fill = (uint16_t)scale_hp;
    }
    else // 208 or more max HP
    {
        temp_bar_length_max = 208; // limit to max 208 pixels

        if (scale_hp_max >= 0x1000000)
        {
            scale_hp >>= 16;
            scale_hp_max >>= 16;
        }
        else if (scale_hp_max >= 0x10000)
        {
            scale_hp >>= 8;
            scale_hp_max >>= 8;
        }

        if (scale_hp_max >= 0x1000)
        {
            scale_hp >>= 4;
            scale_hp_max >>= 4;
        }
        if (scale_hp_max >= 0x400)
        {
            scale_hp >>= 2;
            scale_hp_max >>= 2;
        }
        if (scale_hp_max >= 0x200)
        {
            scale_hp >>= 1;
            scale_hp_max >>= 1;
        }
        if (scale_hp_max >= 0x100)
        {
            scale_hp >>= 1;
            scale_hp_max >>= 1;
        }

        // Adjust the fill based on the fraction using 16-bit division
        temp_bar_length_fill = (uint16_t)((uint16_t)scale_hp * 208) / (uint16_t)scale_hp_max;
    }

    if (raw_hp <= 0)
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
    uint16_t base_attr = 0x2000 | (PAL_UI_4BPP << 10);

    ui_hp_gauge[i++] = 0x016b | base_attr;

    uint16_t filled_end = temp_bar_filled_tiles + 1;
    while (i < filled_end)
    {
        ui_hp_gauge[i++] = 0x0168 | base_attr;
    }

    // Calculate the "behind" backing via lookup table
    uint32_t temp_bar_tile_offset = hp_tile_offsets[temp_bar_length_max & 0x07];

    if (temp_bar_partial_tile != 0)
    {
        if (i == (temp_bar_filled_tiles + temp_bar_empty_tiles + 1))
        {
            // Use the dynamic tile if the partial fill tile is the last tile
            ui_hp_gauge[i] = 0x0169 | base_attr;

            if ((temp_bar_length_max & 0x07) >= 5)
            {
                // One extra tile
                temp_extra_length = 1;
                ui_hp_gauge[i+1] = 0x016a | base_attr;

                if ((DmaSystem_AddItemToQueue(
                    (uint8_t *)(&data_ui_dynamic_hp) + (((temp_bar_length_max & 0x07) + 1) << 5) + temp_bar_tile_offset, 
                    0x56a0, 
                    32,
                    VRAM_INCHIGH, 
                    0 
                    ) != 0 ))
                {
                    return;
                }
            }

            if ((DmaSystem_AddItemToQueue(
                (uint8_t *)(&data_ui_dynamic_hp) + (temp_bar_partial_tile << 5) + temp_bar_tile_offset, 
                0x5690, 
                32,
                VRAM_INCHIGH, 
                0 
                ) != 0 ))
            {
                return;
            }
        }
        else
        {
            ui_hp_gauge[i] = (0x0160 + temp_bar_partial_tile) | base_attr;
        }

        i++;
    }

    if ((temp_bar_length_max & 0x07) == 0 && temp_bar_empty_tiles > 0)
    {
        temp_bar_empty_tiles--;
    }

    uint16_t empty_tile = 0x0160 | base_attr;
    uint16_t empty_end = temp_bar_filled_tiles + temp_bar_empty_tiles + 1;
    while (i < empty_end)
    {
        ui_hp_gauge[i++] = empty_tile;
    }

    if (i == empty_end)
    {
        if ((temp_bar_length_max & 0x07) != 0)
        {
            // last tile is partial
            ui_hp_gauge[i] = 0x0169 | base_attr;

            if ((temp_bar_length_max & 0x07) >= 5)
            {
                temp_extra_length = 1;
                ui_hp_gauge[i+1] = 0x016a | base_attr;

                if (DmaSystem_AddItemToQueue(
                        (uint8_t *)(&data_ui_dynamic_hp) + (((temp_bar_length_max & 0x07) + 1) << 5) + temp_bar_tile_offset, 
                        0x56a0, 
                        32,
                        VRAM_INCHIGH, 
                        0
                        ) != 0 )
                {
                    return;
                }
            }

            if (DmaSystem_AddItemToQueue(
                (uint8_t *)(&data_ui_dynamic_hp) + temp_bar_tile_offset, 
                0x5690, 
                32,
                VRAM_INCHIGH, 
                0
                ) != 0)
            {
                return;
            }
            i++;
        }
        else if (temp_bar_empty_tiles > 0 || temp_bar_filled_tiles < (temp_bar_length_max >> 3))
        {
            ui_hp_gauge[i++] = empty_tile;
        }
    }

    if ((temp_bar_length_max & 0x07) == 0)
    {
        // One extra tile and the last tile was a full tile
        if (temp_bar_empty_tiles > 0 || temp_bar_filled_tiles < (temp_bar_length_max >> 3))
        {
            temp_extra_length = 1;
        }
        ui_hp_gauge[i] = 0x016c | base_attr;
    }

    // Upload the tilemap data itself
    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_hp_gauge[0]), 
        0x3000 + 4 + ((UI_MARGIN_TOP) << 5), 
        (temp_bar_filled_tiles + temp_bar_empty_tiles + 2 + temp_extra_length) << 1,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        ui_cached_hp = raw_hp;
        ui_cached_hp_max = raw_hp_max;
    }

    return;
}

/**
 * @brief Renders an overhead health bar sprite above an enemy object.
 * 
 * @param o Pointer to the target enemy game object.
 */
void UserInterface_DrawEnemyHealthBar(struct game_object * o)
{
    // get fraction of health if it's not changed
    if (o->struct_data.npc_data.hp_cache != o->struct_data.npc_data.hp)
    {
        uint32_t temp_hp = o->struct_data.npc_data.hp;
        uint32_t temp_hp_max = o->struct_data.npc_data.hp_max;

        if (temp_hp_max == 0)
        {
            temp_hp_max = 65535;
        }

        if (temp_hp_max >= 0x1000000)
        {
            temp_hp >>= 16;
            temp_hp_max >>= 16;
        }
        else if (temp_hp_max >= 0x10000)
        {
            temp_hp >>= 8;
            temp_hp_max >>= 8;
        }

        if (temp_hp_max >= 0x1000)
        {
            temp_hp >>= 4;
            temp_hp_max >>= 4;
        }
        if (temp_hp_max >= 0x400)
        {
            temp_hp >>= 2;
            temp_hp_max >>= 2;
        }
        if (temp_hp_max >= 0x200)
        {
            temp_hp >>= 1;
            temp_hp_max >>= 1;
        }
        if (temp_hp_max >= 0x100)
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
        (o->struct_data.npc_data.hp_tile_offset | 3 << 12));

    if (!system_game_paused)
    {
        o->struct_data.npc_data.hp_display_time--;
    }

    return;
}

/**
 * @brief Redraws the player money counter into the UI tilemap buffer.
 */
void UserInterface_UpdateMoneyCounters()
{
    // Copy these values
    uint8_t temp_string[6] = "     " ; // 5 spaces

    int32_t diff = (int32_t)obj_player_pointer->struct_data.npc_data.money - (int32_t)ui_display_money;
    int32_t temp_counter_adjust = diff >> 1; // Amount of money to change visually (avoid 32-bit division)
    if (temp_counter_adjust == 0 && diff != 0)
    {
        temp_counter_adjust = (diff > 0) ? 1 : -1;
    }

    uint32_t temp_money = ui_display_money + temp_counter_adjust;
    uint16_t temp_money_copy;

    // Reduce the visible length of the money.
    // Every digit is a significant cost in displaying the number.
    if (temp_money >= 10000000)
    {
        // at least 10000K
        temp_money_copy = (uint16_t)(temp_money / 1000000);
        temp_string[4] = 'M';
    }
    else if (temp_money >= 10000)
    {
        // at least 10000, exceeding 4 digits counter.
        temp_money_copy = (uint16_t)(temp_money / 1000);
        temp_string[4] = 'K';
    }
    else
    {
        // less than 10K, can be displayed fully
        temp_money_copy = (uint16_t)temp_money;
    }

    // The icon
    ui_money_counter[0] = 0x016d | 0x2000 | (PAL_UI_4BPP << 10);

    uint8_t th = 0, h = 0, t = 0;
    while (temp_money_copy >= 1000)
    {
        temp_money_copy -= 1000;
        th++;
    }
    while (temp_money_copy >= 100)
    {
        temp_money_copy -= 100;
        h++;
    }
    while (temp_money_copy >= 10)
    {
        temp_money_copy -= 10;
        t++;
    }
    uint8_t u = (uint8_t)temp_money_copy;

    if (th != 0)
    {
        temp_string[0] = '0' + th;
        temp_string[1] = '0' + h;
        temp_string[2] = '0' + t;
        temp_string[3] = '0' + u;
    }
    else if (h != 0)
    {
        temp_string[0] = ' ';
        temp_string[1] = '0' + h;
        temp_string[2] = '0' + t;
        temp_string[3] = '0' + u;
    }
    else if (t != 0)
    {
        temp_string[0] = ' ';
        temp_string[1] = ' ';
        temp_string[2] = '0' + t;
        temp_string[3] = '0' + u;
    }
    else
    {
        temp_string[0] = ' ';
        temp_string[1] = ' ';
        temp_string[2] = ' ';
        temp_string[3] = '0' + u;
    }

    uint16_t char_attr_base = 0x20e0 | (PAL_UI_4BPP << 10);
    ui_money_counter[1] = char_attr_base + (uint8_t)temp_string[0];
    ui_money_counter[2] = char_attr_base + (uint8_t)temp_string[1];
    ui_money_counter[3] = char_attr_base + (uint8_t)temp_string[2];
    ui_money_counter[4] = char_attr_base + (uint8_t)temp_string[3];
    ui_money_counter[5] = char_attr_base + (uint8_t)temp_string[4];

    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_money_counter[0]), 
        0x3000 + 1 + ((27 - UI_MARGIN_TOP) << 5), 
        12,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        if (temp_money == obj_player_pointer->struct_data.npc_data.money)
        {
            ui_cached_money = temp_money;
        }
    }

    ui_display_money = temp_money;

    return;
}


/**
 * @brief Formats a 16-bit unsigned integer into a 3-digit ASCII string buffer.
 * 
 * @param buf Target string buffer.
 * @param val Value to format.
 */
void UserInterface_Internal_Format3U(char *buf, uint16_t val)
{
    if (val > 999)
    {
        val = 999;
    }

    uint8_t h = 0;
    while (val >= 100)
    {
        val -= 100;
        h++;
    }

    uint8_t t = 0;
    while (val >= 10)
    {
        val -= 10;
        t++;
    }

    uint8_t u = (uint8_t)val;

    if (h != 0)
    {
        buf[0] = (char)('0' + h);
        buf[1] = (char)('0' + t);
        buf[2] = (char)('0' + u);
    }
    else if (t != 0)
    {
        buf[0] = ' ';
        buf[1] = (char)('0' + t);
        buf[2] = (char)('0' + u);
    }
    else
    {
        buf[0] = ' ';
        buf[1] = ' ';
        buf[2] = (char)('0' + u);
    }

    return;
}

/**
 * @brief Redraws the level enemy kill count into the UI tilemap buffer.
 */
void UserInterface_UpdateEnemyCounters()
{
    // Copy these values
    char temp_string[8];
    uint16_t base_attr = 0x2000 | (PAL_UI_4BPP << 10);

    ui_enemy_counter[0] = 0x016e | base_attr;

    UserInterface_Internal_Format3U(&temp_string[0], obj_enemies_defeated);
    temp_string[3] = '/';
    UserInterface_Internal_Format3U(&temp_string[4], obj_enemies_max_count);

    uint16_t char_attr_base = 0x20e0 | (PAL_UI_4BPP << 10);
    ui_enemy_counter[1] = char_attr_base + (uint8_t)temp_string[0];
    ui_enemy_counter[2] = char_attr_base + (uint8_t)temp_string[1];
    ui_enemy_counter[3] = char_attr_base + (uint8_t)temp_string[2];
    ui_enemy_counter[4] = char_attr_base + (uint8_t)temp_string[3];
    ui_enemy_counter[5] = char_attr_base + (uint8_t)temp_string[4];
    ui_enemy_counter[6] = char_attr_base + (uint8_t)temp_string[5];
    ui_enemy_counter[7] = char_attr_base + (uint8_t)temp_string[6];

    if (obj_enemies_defeated >= obj_enemies_target_count)
    {
        ui_level_status[0] = 0x0195 | base_attr;
        ui_level_status[1] = 0x0196 | base_attr;
        ui_level_status[2] = 0x0197 | base_attr;
        ui_level_status[3] = 0x0198 | base_attr;
        ui_level_status[4] = 0x0199 | base_attr;
    }
    else
    {
        uint16_t blank = 0x0100 | base_attr;
        ui_level_status[0] = blank;
        ui_level_status[1] = blank;
        ui_level_status[2] = blank;
        ui_level_status[3] = blank;
        ui_level_status[4] = blank;
    }

    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_level_status[0]), 
        0x3000 + 0x12 + ((27 - UI_MARGIN_TOP) << 5),  
        10,
        VRAM_INCHIGH, 
        0
        ) != 0)
    {
        return;
    }

    if (DmaSystem_AddItemToQueue(
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


/**
 * @brief Prints an 8x8 fixed-width text string directly into the UI tilemap buffer.
 * 
 * @param string_ptr Pointer to the null-terminated string.
 * @param row        Target tilemap row.
 * @param col        Target tilemap column.
 */
void UserInterface_PrintText(char * string_ptr, uint16_t row, uint16_t col)
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

    // Clear the line first!
    UserInterface_ClearTextBuffer_Subset(row, col, 32);
    
    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_text[0][0]), 
        0x3400 + (row << 5) + (col), 
        temp_len,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        ui_show_message_cleared = false;
        ui_show_message_ttl = 60 / V_MUL;
    }
    return;
}

/**
 * @brief Prints an 8x8 fixed-width text string in Mode 3 configuration.
 * 
 * @param string_ptr Pointer to the null-terminated string.
 * @param row        Target tilemap row.
 * @param col        Target tilemap column.
 */
void UserInterface_PrintText_Mode3(char * string_ptr, uint16_t row, uint16_t col)
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

    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_text[0][0]), 
        0x4c00 + (row << 5) + (col), 
        temp_len,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        ui_show_message_cleared = false;
        ui_show_message_ttl = 60 / V_MUL;
    }
    return;
}

/**
 * @brief Clears the UI window background tilemap buffer.
 * 
 * @param use_clear_tile If true, fills the buffer with clear tile IDs; if false, clears to zero.
 */
void UserInterface_ClearWindowBuffer(bool use_clear_tile)
{
    volatile uint16_t vwf_tile_id_copy = vwf_tile_id_empty;

    if (use_clear_tile)
    {
        vwf_tile_id_empty = 0x0100 | 0x2000 | (PAL_UI_4BPP << 10);
    }
    else
    {
        vwf_tile_id_empty = 0x015f | 0x2000 | (PAL_UI_4BPP << 10);
    }
    
    VwfEngine_PrintText_ResetTilemap((uint16_t *)&ui_window_background[0][0], 32 * (SCREEN_HEIGHT >> 3));
    vwf_tile_id_empty = vwf_tile_id_copy;

    return;
}

/**
 * @brief Clears the UI text tilemap buffer (`ui_window_text`).
 */
void UserInterface_ClearTextBuffer()
{
    volatile uint16_t vwf_tile_id_copy = vwf_tile_id_empty;
    vwf_tile_id_empty = 0x0000;
    VwfEngine_PrintText_ResetTilemap((uint16_t *)&ui_window_text[0][0], 32 * (SCREEN_HEIGHT >> 3));
    vwf_tile_id_empty = vwf_tile_id_copy;

    return;
}

/**
 * @brief Clears a single row of tiles in the UI text buffer.
 * 
 * @param y Target tilemap row index.
 */
void UserInterface_ClearTextBuffer_Line(uint16_t y)
{
    volatile uint16_t vwf_tile_id_copy = vwf_tile_id_empty;
    vwf_tile_id_empty = 0x0000;
    VwfEngine_PrintText_ResetTilemap((uint16_t *)&ui_window_text[y][0], 32);
    vwf_tile_id_empty = vwf_tile_id_copy;

    return;
}


/**
 * @brief Draws a bordered, more graphically decorated window box.
 * 
 * @param x Column origin.
 * @param y Row origin.
 * @param w Width in tiles.
 * @param h Height in tiles.
 */
void UserInterface_DrawWindowBackground(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    // Sanity checks
    if ((w == 0) || (h == 0)) // Both must be tested separately
    {
        // Size is 0
        return;
    }
    if ((x >= 32) || (y >= 32))
    {
        // Start of window is outside the screen
        return;
    }

    uint16_t end_y = y + h;

    // Check that the end of the window is not outside the screen. If it is, truncate it to the screen size.
    if (end_y > (SCREEN_HEIGHT >> 3))
    {
        end_y = (SCREEN_HEIGHT >> 3);
    }

    uint16_t tile_attr = 0x2000 | (PAL_UI_4BPP << 10);
    uint16_t w_is_even = ((w & 1) == 0);

    for (unsigned int i = y; i < end_y; i++)
    {
        uint16_t row_index = i - y;
        uint16_t base_tile;

        if (row_index == 0)
        {
            // First row
            base_tile = 0x0170;
        }
        else if (row_index == h - 1)
        {
            // Last row
            base_tile = (h & 1) ? 0x01b0 : 0x01a0;
        }
        else if (row_index == 1)
        {
            // Second row
            base_tile = 0x0180;
        }
        else if (row_index == h - 2)
        {
            // Second to last row
            base_tile = 0x0190;
        }
        else if (row_index & 1)
        {
            // Odd row
            base_tile = 0x0180;
        }
        else
        {
            // Even row
            base_tile = 0x0190;
        }

        uint16_t *row_ptr = ui_window_background[i];
        
        // Draw first column
        row_ptr[x] = base_tile | tile_attr;

        if (w > 1)
        {
            // Draw rightmost column
            uint16_t right_tile = w_is_even ? 3 : 4;
            row_ptr[x + w - 1] = (base_tile + right_tile) | tile_attr;

            // Draw middle columns
            if (w > 2)
            {
                row_ptr[x + 1] = (base_tile + 1) | tile_attr;
                if (w > 3)
                {
                    row_ptr[x + 2] = (base_tile + 2) | tile_attr;
                    
                    int16_t block_len = (w - 4) << 1;
                    if (block_len >= 2)
                    {
                        // Fill the rest using a copy
                        System_CopyBlock((uint8_t *)&row_ptr[x + 1], (uint8_t *)&row_ptr[x + 3], block_len);
                    }
                }
            }
        }
    }
}

/**
 * @brief Draws a solid window background fill rectangle into the UI buffer.
 * 
 * @param x Column origin.
 * @param y Row origin.
 * @param w Width in tiles.
 * @param h Height in tiles.
 */
void UserInterface_DrawWindowBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    // Sanity checks
    if ((w == 0) || (h == 0))
    {
        // Size is 0
        return;
    }
    if ((x >= 32) || (y >= (SCREEN_HEIGHT >> 3)))
    {
        // Start of window is outside the screen
        return;
    }

    // Check that the end of the window is not outside the screen. If it is, truncate it to the screen size.
    uint16_t end_y = y + h;
    if (end_y > (SCREEN_HEIGHT >> 3))
    {
        end_y = (SCREEN_HEIGHT >> 3);
    }

    uint16_t tile_attr = 0x2000 | (PAL_UI_4BPP << 10);

    for (unsigned int i = y; i < end_y; i++)
    {
        uint16_t row_index = i - y;
        uint16_t base_tile;

        if (row_index == 0)
        {
            // First row
            base_tile = 0x018a;
        }
        else if (row_index == h - 1)
        {
            // Last row
            base_tile = 0x01aa;
        }
        else
        {
            // Neither
            base_tile = 0x019a;
        }

        uint16_t *row_ptr = ui_window_background[i];

        // Draw first column
        row_ptr[x] = base_tile | tile_attr;

        if (w > 1)
        {
            // Draw rightmost column
            row_ptr[x + w - 1] = (base_tile + 2) | tile_attr;

            // Draw middle columns
            if (w > 2)
            {
                row_ptr[x + 1] = (base_tile + 1) | tile_attr;
                
                int16_t block_len = (w - 3) << 1;
                if (block_len >= 2)
                {
                    // Fill the rest using a copy
                    System_CopyBlock((uint8_t *)&row_ptr[x + 1], (uint8_t *)&row_ptr[x + 2], block_len);
                }
            }
        }
    }
}

/**
 * @brief Draws window text at specified tile coordinates.
 * 
 * @param string_ptr Pointer to the null-terminated string.
 * @param x          Column tile offset.
 * @param y          Row tile offset.
 */
void UserInterface_DrawWindowText(char * string_ptr, uint16_t x, uint16_t y)
{
    // Sanity check
    if ((x >= 32) || (y >= (SCREEN_HEIGHT >> 3)))
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
            continue;
        }
        else if (col >= 32)
        {
            row++;
            col = x;
        }
        
        if (row >= (SCREEN_HEIGHT >> 3))
        {
            break;
        }

        ui_window_text[row][col] = (-0x20 + *string_ptr++) | 0x2000  | (PAL_UI_TEXT_WHITE << 10);

        col++;
    }

    return;
}

/**
 * @brief Enqueues UI background and text tilemap buffers into the DMA transfer queue for VRAM update.
 */
void UserInterface_CopyUiBuffers()
{
    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_background[0][0]), 
        0x3000, 
        (SCREEN_HEIGHT / 4) * 32,
        VRAM_INCHIGH, 
        0
        ) != 0)
    {
        return;
    }

    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_text[0][0]), 
        0x3400, 
        (SCREEN_HEIGHT / 4) * 32,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        return;
    }

    return;
}

/**
 * @brief Enqueues a single row of the UI text buffer for DMA transfer to VRAM.
 * 
 * @param y Target row index.
 */
void UserInterface_CopyTextBuffer_Line(uint16_t y)
{
    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_text[y][0]), 
        0x3400 + (y << 5), 
        64,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        return;
    }

    return;
}

/**
 * @brief Directly copies UI tileset graphics into VRAM.
 */
void UserInterface_CopyUiGraphicsToVram()
{
    DmaSystem_AddItemToQueue((uint8_t *)((uint32_t)&data_ui_dynamic_textadvance + ((uint16_t)((system_frames_elapsed >> 2) & 0x03) << 5)), 0x4300, 32, VRAM_INCHIGH, 0);
    DmaSystem_AddItemToQueue((uint8_t *)((uint32_t)&data_ui_dynamic_textadvance + 256 + ((uint16_t)((system_frames_elapsed >> 2) & 0x03) << 5)), 0x4380, 32, VRAM_INCHIGH, 0);
    
    return;
}