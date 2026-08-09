#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"
#include "system.h"
#include "loop.h"

#if VBCC_ASM == 0
void Loop_Subscreen_MapDisplay_InitBackground(void)
{
    REG_VMAIN = VRAM_INCHIGH;
    REG_VMADDLH = TILEMAP_ADDR_MAP_UI;

    for (int l = 0; l < 1024; l++)
    {
        REG_VMDATALH = 256;
    }

    return;
}
#endif
