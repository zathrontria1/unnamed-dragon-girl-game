#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <snes/console.h>

#include "vars.h"

#include "gfx.h"
#include "dma.h"
#include "system.h"
#include "lz4.h"

#include "errorhandling.h"

void ErrorHandler_Controller()
{
    // Set up the PPU regs to what we want.
    System_Init_BgScroll();

    REG_BGMODE = 0x09; // Mode 1, high priority bg3
    REG_TM = 0x01; // BG1 only
    REG_BG12NBA = 0 << 4 | 0;

    REG_BG1SC = 0x3800 >> 8; // The image is 28KB. Have the tilemap go to the 28Kth byte.

    // Decompress the splash and tilemap
    LZ4_UnpackToWRAM((void *)&data_bg_error_controller_lz4, 0x007f0000);
    LZ4_UnpackToWRAM((void *)&data_tilemap_error_controller_lz4, 0x007f7000);
    
    // Copy the palette
    DmaSystem_CopyToWram((uint32_t)data_palette_splash, (uint32_t)&shadow_cgram, 256);

    // Upload the splash
    DmaSystem_CopyToVram(0x007f0000, 0x0000, 0x7800); // Copy the entire section including the tilemap.
    DmaSystem_UploadCgram();

    // Set up a fade-in. Doing this so that we can actually run the other steps
    // while the game is still setting up.
    shadow_inidisp_change = 1;
    gfx_mosaic_change = -1;
    gfx_mosaic_layers = 0x01; // BG1
    gfx_mosaic_intensity = 16; // Max intensity
    system_use_alternate_nmi = 1;

    shadow_inidisp = 0x00;

    System_EnableInterrupts();

    // Lock up the system
    exit(EXIT_FAILURE);
}