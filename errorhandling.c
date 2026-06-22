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

    uint16_t transfer_length = VwfEngine_PrintText((uint8_t *)&STR_ERROR_CONTROLLER, (uint8_t *)LZ4_BUFFER_ADDR, (uint16_t *)(LZ4_BUFFER_ADDR+0x7000), 1, 1, 0);
    
    // Copy the palette
    DmaSystem_CopyToWram((uint32_t)data_palette_ui, (uint32_t)&shadow_cgram, 32);

    // Upload the error message
    DmaSystem_CopyToVram((uint32_t)LZ4_BUFFER_ADDR, 0x0000, transfer_length); // Copy the entire section including the tilemap.
    DmaSystem_CopyToVram((uint32_t)(LZ4_BUFFER_ADDR+0x7000), 0x3800, 1792); // Copy the entire section including the tilemap.
    DmaSystem_UploadCgram();

    //VwfEngine_PrintText_Gradual_Setup((uint8_t *)&STR_ERROR_CONTROLLER, (uint8_t *)LZ4_BUFFER_ADDR, (uint16_t *)(LZ4_BUFFER_ADDR+0x7000), 1, 1, 0);

    //DmaSystem_AddItemToQueue((uint8_t *)(LZ4_BUFFER_ADDR), 0x0000, 16, VRAM_INCHIGH, 0);
    //DmaSystem_AddItemToQueue((uint8_t *)(LZ4_BUFFER_ADDR+0x7000), 0x3800, 1792, VRAM_INCHIGH, 0);

    DmaSystem_ProcessQueue();

    // Set up a fade-in
    shadow_inidisp_change = 1;

    gfx_mosaic_change = 0;
    gfx_mosaic_layers = 0; // Off
    gfx_mosaic_intensity = 0; // Off
    system_use_alternate_nmi = 1;

    shadow_inidisp = 0x00;

    //uint8_t * wram_offset = (uint8_t *)LZ4_BUFFER_ADDR;

    HdmaEngine_SetHdmaShadow();

    System_EnableInterrupts();

    REG_TM = 0x04; // BG3 only

    // Now that tests are done...
    /*System_WaitUntilVblank();

    uint16_t vram_offset = 0x0008;

    while (1)
    {
        

        if (vwf_print_finished)
        {
            break;
        }

        uint8_t * new_data_addr = VwfEngine_PrintText_Gradual(1);

        DmaSystem_AddItemToQueue((uint8_t *)(LZ4_BUFFER_ADDR+0x7000), 0x3800, 1792, VRAM_INCHIGH, 0);

        if (vwf_tiledata_run != 0)
        {
            register volatile unsigned int data_len = vwf_tiledata_run;
            DmaSystem_AddItemToQueue(new_data_addr, vram_offset, data_len, VRAM_INCHIGH, 0);

            wram_offset = wram_offset + vwf_tiledata_advance;
            vram_offset = vram_offset + vwf_tiledata_advance_vram;
        }

        System_WaitUntilVblank();
    }*/

    // Lock up the system
    exit(EXIT_FAILURE);
}