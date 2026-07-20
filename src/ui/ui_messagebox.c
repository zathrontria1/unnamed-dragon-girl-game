#include "snes/console.h"

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "dma.h"

#include "ui.h"
#include "ui_messagebox.h"
#include "ui_vwf.h"
#include "spr.h"

#include "snd.h"
#include "consts_snd.h"

// These functions are currently separated out for organization purposes
// Quite a bunch of them are now calls to other more generic functions

/**
 * @brief Clears a linear range of tiles in the text buffer starting at (row, col).
 * 
 * @param row Target tilemap row.
 * @param col Target tilemap column.
 * @param len Number of tiles to clear.
 */
void UserInterface_ClearTextBuffer_Subset(uint16_t row, uint16_t col, uint16_t len)
{
    if (len == 0)
    {
        return;
    }

    if (row >= (SCREEN_HEIGHT >> 3))
    {
        return;
    }

    if (col >= 32)
    {
        return;
    }

    volatile uint16_t vwf_tile_id_copy = vwf_tile_id_empty;
    vwf_tile_id_empty = 0x0000;
    VwfEngine_PrintText_ResetTilemap((uint16_t *)&ui_window_text[row][col], len);
    vwf_tile_id_empty = vwf_tile_id_copy;
    
    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_text[row][col]), 
        0x3400 + (row << 5) + (col), 
        len << 1,
        VRAM_INCHIGH, 
        0
        ) == 0)
    {
        ui_show_message_cleared = true;
    }

    return;
}

/**
 * @brief Draws a full-width dialog textbox window background across the screen.
 * 
 * @param row Starting row offset.
 * @param h   Height of the textbox in tiles (including borders).
 */
void UserInterface_DrawTextbox(uint16_t row, uint16_t h)
{
    UserInterface_DrawWindowBackground(0, 0, 32, h);

    UserInterface_CopyTextboxToVram(row, h);

    return;
}


/**
 * @brief Clears a full-width dialog textbox window background and text.
 * 
 * @param row Starting row offset.
 * @param h   Height of the textbox in tiles.
 */
void UserInterface_ClearTextbox(uint16_t row, uint16_t h)
{
    volatile uint16_t vwf_tile_id_copy = vwf_tile_id_empty;
    vwf_tile_id_empty = 0x0100;
    VwfEngine_PrintText_ResetTilemap((uint16_t *)&ui_window_background, 192);
    vwf_tile_id_empty = vwf_tile_id_copy;

    // Only one memclear DMA can be done per logical frame, so this workaround is needed.
    UserInterface_CopyTextboxToVram(row, h);
    UserInterface_ClearTextboxText(row, h);

    return;
}

/**
 * @brief Clears the text portion of a dialog textbox via VRAM fill DMA.
 * 
 * @param row Starting row offset.
 * @param h   Height of the textbox in tiles.
 */
void UserInterface_ClearTextboxText(uint16_t row, uint16_t h)
{
    DmaSystem_SetClear(0x3400 + ((row + 1) << 5), h << 6, 0x00); // Add one extra row for the indicator overhang

    return;
}

/**
 * @brief Enqueues a full-width dialog textbox tilemap to the DMA transfer queue.
 * 
 * @param row Starting row offset.
 * @param h   Height of the textbox in tiles.
 */
void UserInterface_CopyTextboxToVram(uint16_t row, uint16_t h)
{
    DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_background[0][0]), 
        0x3000 + (row << 5), 
        (h << 6),
        VRAM_INCHIGH, 
        0
        );
    
    return;
}

// Used to display the textbox advance indicator.
const uint16_t const_ui_textadvance_tilemapentries[] =
{
    0x0060 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
    0x0061 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
    0x0070 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
    0x0071 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
};
