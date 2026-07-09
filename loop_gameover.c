#include <snes/console.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "data_strings.h"

#include "lz4.h"
#include "system.h"

#include "gfx.h"
#include "dma.h"
#include "hdma.h"
#include "ui_vwf.h"
#include "errorhandling.h"

#include "loop_gameover.h"

void GameOver_Loop()
{
    ErrorHandler_Internal_Setup();
    ErrorHandler_Internal_Display((uint8_t *)&STR_GAME_OVER);

    int timer = 10 * FPS;

    while (1)
    {
        System_WaitUntilVblank();

        if (timer > 0)
        {
            timer--;

            if (timer <= (16 * V_MUL) - 1)
            {
                shadow_brightness = (timer << 8) / V_MUL;
            }
        }

        if (!timer)
        {
            System_SoftReset();
        }
    }

    exit(EXIT_SUCCESS);
}