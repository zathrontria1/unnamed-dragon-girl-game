#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "snes/console.h"

#include "vars.h"

#include "gfx.h"
#include "dma.h"
#include "system.h"
#include "lz4.h"
#include "hdma.h"

#include "data_strings.h"

#include "ui_vwf.h"
#include "ui_textscreen.h"

#include "errorhandling.h"

#include "sram_management.h"

void ErrorHandler_Controller()
{
    Ui_TextScreen_Setup();

    Ui_TextScreen_Display((uint8_t *)&STR_ERROR_CONTROLLER);

    exit(EXIT_FAILURE);
}

void ErrorHandler_Region()
{
    Ui_TextScreen_Setup();

    Ui_TextScreen_Display((uint8_t *)&STR_ERROR_REGION);

    uint8_t * string_ptr = (uint8_t *)0x007ffff8;
    for (int i = 0; i < 8; i++)
    {
        *string_ptr = const_sram_verify_str[i];
        string_ptr++;
    }
    
    while (1)
    {
        // Let the player get out of this
        System_WaitUntilVblank();
    }

    // Should be unreachable
    exit(EXIT_FAILURE + 1);
}
