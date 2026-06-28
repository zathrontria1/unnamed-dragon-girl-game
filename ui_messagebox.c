#include <snes/console.h>

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "dma.h"

#include "ui.h"
#include "ui_messagebox.h"
#include "spr.h"

#include "snd.h"
#include "consts_snd.h"

// These functions are currently separated out for organization purposes
// Quite a bunch of them are now calls to other more generic functions

// Starting x and y coord, followed by length to clear, linearly (this will not clear columns)
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

    int i = 0;
    bool overflow = false;
    for (int y = row; y < (SCREEN_HEIGHT >> 3); y++)
    {
        for (int x = col; x < 32; x++)
        {
            ui_window_text[y][x] = 0x0000 | 0x2000 | (PAL_UI_TEXT_WHITE << 10);
            i++;

            if (i >= len)
            {
                overflow = true;
                break;
            }
        }

        if (overflow == true)
        {
            break;
        }
    }
    
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

/* 
    Textbox border drawing and clearing functions

    Note that they always take the full width of the screen.

    Height includes borders (e.g. 4 text rows = h is 6)

    Note: this now calls to the more generic versions
*/
void UserInterface_DrawTextbox(uint16_t row, uint16_t h)
{
    UserInterface_DrawWindowBackground(0, 0, 32, h);

    UserInterface_CopyTextboxToVram(row, h);

    return;
}


void UserInterface_ClearTextbox(uint16_t row, uint16_t h)
{
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            ui_window_background[i][j] = 0x0100; // Empty tile for 4bpp
        }
    }

    // Only one memclear DMA can be done per logical frame, so this workaround is needed.
    UserInterface_CopyTextboxToVram(row, h);
    UserInterface_ClearTextboxText(row, h);

    return;
}

void UserInterface_ClearTextboxText(uint16_t row, uint16_t h)
{
    DmaSystem_SetClear(0x3400 + ((row + 1) << 5), (h - 1) << 6);

    return;
}

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
