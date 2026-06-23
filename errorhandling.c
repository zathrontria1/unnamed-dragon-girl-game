#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <snes/console.h>

#include "vars.h"

#include "gfx.h"
#include "dma.h"
#include "system.h"
#include "lz4.h"
#include "hdma.h"

#include "data_strings.h"

#include "ui_vwf.h"

#include "errorhandling.h"

void ErrorHandler_Controller()
{
    ErrorHandler_Internal_Setup();

    ErrorHandler_Internal_Display((uint8_t *)&STR_ERROR_CONTROLLER);

    exit(EXIT_FAILURE);
}

void ErrorHandler_Region()
{
    ErrorHandler_Internal_Setup();

    ErrorHandler_Internal_Display((uint8_t *)&STR_ERROR_REGION);

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

void ErrorHandler_Internal_Setup()
{
    // Disable interrupts
    System_DisableInterrupts();

    // Disable HDMA and screen
    REG_HDMAEN = 0x00;
    REG_INIDISP = 0x8f;

    // Set up the PPU regs to what we want.
    System_Init_BgScroll();

    REG_BGMODE = 0x09; // Mode 1, high priority bg3
    REG_TM = 0x04; // BG3 only
    REG_BG34NBA = 0 << 4 | 0;

    REG_BG3SC = 0x3800 >> 8; // The image is 14KB. Have the tilemap go to the 28Kth byte.

    return;
}

void ErrorHandler_Internal_Display(uint8_t * string_ptr)
{
    uint16_t transfer_length = VwfEngine_PrintText(string_ptr, (uint8_t *)LZ4_BUFFER_ADDR, (uint8_t *)(LZ4_BUFFER_ADDR+0x7000), 1, 1, 0);
    
    // Copy the palette
    DmaSystem_CopyToWram((uint32_t)data_palette_ui, (uint32_t)&shadow_cgram, 32);

    // Upload the error message
    DmaSystem_CopyToVram((uint32_t)LZ4_BUFFER_ADDR, 0x0000, transfer_length); // Copy the entire section including the tilemap.
    DmaSystem_CopyToVram((uint32_t)(LZ4_BUFFER_ADDR+0x7000), 0x3800, 1792); // Copy the entire section including the tilemap.
    DmaSystem_UploadCgram();

    DmaSystem_ProcessQueue();

    // Set up a fade-in
    shadow_inidisp_change = 1;

    gfx_mosaic_change = 0;
    gfx_mosaic_layers = 0; // Off
    gfx_mosaic_intensity = 0; // Off
    system_use_alternate_nmi = 1;

    shadow_inidisp = 0x00;

    HdmaEngine_SetHdmaShadow();

    System_EnableInterrupts();

    REG_TM = 0x04; // BG3 only

    return;
}
