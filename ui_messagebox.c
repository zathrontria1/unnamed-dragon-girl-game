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

// These functions are currently separated out for organization purposes
// Some of these will also be edited later as they're redundant at times...

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
