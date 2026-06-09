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

uint16_t ui_show_message_char_col;
uint16_t ui_show_message_char_row;

bool ui_show_message_finished; // true if the current page of message is fully printed

uint16_t ui_show_message_linewidth[4]; // Printable text length of a given line.

// These functions are currently separated out for organization purposes
// Some of these will also be edited later as they're redundant at times...

// Starting x and y coord, followed by length to clear, linearly (this will not clear columns)
void UserInterface_ClearTextBuffer_Subset(uint16_t row, uint16_t col, uint16_t len)
{
    if (len == 0)
    {
        return;
    }

    if (row >= 32)
    {
        return;
    }

    if (col >= 32)
    {
        return;
    }

    int i = 0;
    bool overflow = false;
    for (int y = row; y < 32; y++)
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

    ui_show_message_linewidth[0] = 0;
    ui_show_message_linewidth[1] = 0;
    ui_show_message_linewidth[2] = 0;
    ui_show_message_linewidth[3] = 0;
    
    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_text[row][col]), 
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
    ui_show_message_finished = false;
    ui_show_message_char_col = 0;
    ui_show_message_char_row = 0;

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

    // Clear the tilemap section
    if (DmaSystem_SetClear(0x3400 + ((UI_MSGBOX_ML_START + 1) << 5), 384) == 1)
    {
        return;
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
                ui_show_message_linewidth[j] = i;

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
                ui_show_message_linewidth[j] = i;

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
            ui_show_message_linewidth[temp_row] = temp_col;

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
            ui_show_message_linewidth[temp_row] = 30;

            temp_col = 0;
            temp_row++; // This might be followed by...
        }

        // Test this right before printing.
        if (temp_row >= 4)
        {
            // Incorrectly formatted text - row overflow
            // Assume that the string has ended at this point.

            ui_show_message_linewidth[3] = 30; // Just in case

            ui_show_message_page = 0;
            break;
        }

        // Normal printable.
        ui_window_text[temp_row][temp_col] = (-0x0020 + *string_ptr) | 0x2000 | (PAL_UI_TEXT_WHITE << 10);

        temp_col++;
        string_ptr++;
    }

    ui_show_message_page_ptr = string_ptr;

    // Copy the text piecemeal based on the line width array later, not here

    /*
    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_text[0][0]), 
        0x3400 + ((row + 1) << 5) + (col), 
        252,
        VRAM_INCHIGH, 
        0
        ) == 1)
        {
            return;
        }
    */

    ui_show_message_cleared = 0;

    SoundInterface_PlayClip(STREAM_TYPEWRITER);
    
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
        if (DmaSystem_AddItemToQueue(
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

/*
    Prints a character to the text box

    Uses global variables
*/
void UserInterface_PrintText_PerChar()
{
    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_text[ui_show_message_char_row][ui_show_message_char_col]), 
        0x3400 + ((ui_show_message_char_row + 1 + UI_MSGBOX_ML_START) << 5) + (ui_show_message_char_col + 1), 
        2 * V_MUL,
        VRAM_INCHIGH, 
        0
        ) == 1)
        {
            return;
        }

    ui_show_message_char_col += 1 * V_MUL; // 1 char per 60FPS frame / 2 char per 30FPS frame
    while (ui_show_message_char_col >= ui_show_message_linewidth[ui_show_message_char_row])
    {
        ui_show_message_char_row++;
        ui_show_message_char_col = 0;

        if (ui_show_message_char_row >= 4)
        {
            ui_show_message_finished = true;
            
            SoundInterface_StopStream();

            ui_show_message_char_col = 0;
            ui_show_message_char_row = 0;

            // These are for the text advance indicator
            if (DmaSystem_AddItemToQueue(
                (uint8_t *)(&const_ui_textadvance_tilemapentries[0]), 
                0x3400 + ((UI_MSGBOX_ML_START + 5) << 5) + (30), 
                4,
                VRAM_INCHIGH, 
                0
                ) == 1)
                {
                    return;
                }

            if (DmaSystem_AddItemToQueue(
                (uint8_t *)(&const_ui_textadvance_tilemapentries[2]), 
                0x3400 + ((UI_MSGBOX_ML_START + 6) << 5) + (30), 
                4,
                VRAM_INCHIGH, 
                0
                ) == 1)
                {
                    return;
                }

            break;
        }
    }

    return;
}

/*
    Called to forcibly print the entire contents if the user presses A mid-printing.
*/
void UserInterface_PrintText_All()
{
    SoundInterface_StopStream();

    if (DmaSystem_AddItemToQueue(
        (uint8_t *)(&ui_window_text[0][0]), 
        0x3400 + ((1 + UI_MSGBOX_ML_START) << 5) + 1, 
        254,
        VRAM_INCHIGH, 
        0
        ) == 1)
        {
            return;
        }

        ui_show_message_finished = true;
        ui_show_message_char_col = 0;
        ui_show_message_char_row = 0;

        // These are for the text advance indicator
        if (DmaSystem_AddItemToQueue(
            (uint8_t *)(&const_ui_textadvance_tilemapentries[0]), 
            0x3400 + ((UI_MSGBOX_ML_START + 5) << 5) + (30), 
            4,
            VRAM_INCHIGH, 
            0
            ) == 1)
            {
                return;
            }

        if (DmaSystem_AddItemToQueue(
            (uint8_t *)(&const_ui_textadvance_tilemapentries[2]), 
            0x3400 + ((UI_MSGBOX_ML_START + 6) << 5) + (30), 
            4,
            VRAM_INCHIGH, 
            0
            ) == 1)
            {
                return;
            }


    return;
}


void UserInterface_CopyTextboxToVram(uint16_t row, uint16_t h)
{
    if (DmaSystem_AddItemToQueue(
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
        if (DmaSystem_AddItemToQueue(
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

    if (DmaSystem_AddItemToQueue(
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
