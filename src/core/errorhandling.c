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

#include "errorhandling.h"

#include "sram_management.h"

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
    REG_TM = 0x05; // BG1/3 only
    REG_BG12NBA = 0 << 4 | 0;
    REG_BG34NBA = 4 << 4 | 4;

    REG_BG1SC = 0x3800 >> 8;
    REG_BG3SC = 0x3c00 >> 8;

    return;
}

void ErrorHandler_Internal_Display(uint8_t * string_ptr)
{
    uint16_t transfer_length_1 = LZ4_UnpackToWRAM(&data_bg_error_back_lz4, (uint8_t *)LZ4_BUFFER_ADDR);
    LZ4_UnpackToWRAM(&data_tilemap_error_back_lz4, (uint8_t *)(LZ4_BUFFER_ADDR+0xc800));
    uint16_t transfer_length_2 = VwfEngine_PrintText(string_ptr, (uint8_t *)(LZ4_BUFFER_ADDR+0x8000), (uint8_t *)(LZ4_BUFFER_ADDR+0xc000), 1, 1, 0);
    
    // Copy the palette
    DmaSystem_CopyToWram((uint8_t *)&data_palette_ui, (uint8_t *)&shadow_cgram, 32);
    DmaSystem_CopyToWram((uint8_t *)&data_palette_error, (uint8_t *)((uint32_t)&shadow_cgram+32), 224);

    // Upload the error message
    DmaSystem_CopyToVram((uint8_t *)(LZ4_BUFFER_ADDR), 0x0000, transfer_length_1);
    DmaSystem_CopyToVram((uint8_t *)(LZ4_BUFFER_ADDR+0xc800), 0x3800, 1792);

    DmaSystem_CopyToVram((uint8_t *)(LZ4_BUFFER_ADDR+0x8000), 0x4000, transfer_length_2);
    DmaSystem_CopyToVram((uint8_t *)(LZ4_BUFFER_ADDR+0xc000), 0x3c00, 1792);
    DmaSystem_UploadCgram();

    DmaSystem_ProcessQueue();

    // Set up a fade-in
    shadow_brightness_change = (64 * V_MUL);
    shadow_brightness = 0 << 8;
    shadow_fblank_enable = 0x00;

    gfx_mosaic_change = 0;
    gfx_mosaic_layers = 0; // Off
    gfx_mosaic_intensity = 0; // Off
    system_use_alternate_nmi = true;

    

    HdmaEngine_SetHdmaShadow();

    System_EnableInterrupts();

    return;
}
