#include "snes/console.h"

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "map.h"

#include "obj.h"

#include "dma.h"
#include "system.h"

#include "ui.h"
#include "ui_messagebox.h"
#include "ui_vwf.h"
#include "spr.h"

#include "lz4.h"

#include "crash_handler.h"

#include "data_strings.h"

// Scratch area used by crash handler.
uint16_t crashhandler_a;
uint16_t crashhandler_x;
uint16_t crashhandler_y;

uint8_t crashhandler_flags;

uint32_t crashhandler_pc;
uint16_t crashhandler_sp;

uint16_t crashhandler_directpage;
uint8_t crashhandler_databank;

// Treat pseudoregs as 32-bit regs
uint32_t crashhandler_regs[16];
uint32_t crashhandler_regs_float[4];

uint16_t crashhandler_stack[16];

uint32_t crashhandler_emulation_mode; // This must be 4 bytes wide to avoid clobbering the next bytes over in case

/*
    Handle the rest of the crash here. Jump into this from assembly code.
*/
void System_CrashHandler_Followup()
{
    REG_INIDISP = 0x8f; // enable forced blank

    // Now we should have free reign in video memory. Re-init.
    System_Init_CpuRegs();
    System_Init_WramFunctions();

    System_Init_BgScroll();

    System_Init_UiTilemap();

    REG_BGMODE = 0x09; // Mode 1, high priority bg3
    REG_TM = 0x04; // BG3 only

    REG_BG34NBA = (4 << 4 | 4);
    REG_BG3SC = TILEMAP_ADDR_GAME_UI_2BPP >> 8;

    DmaSystem_CopyToWram((uint8_t *)&data_palette_ui, (uint8_t *)&shadow_cgram, 32);
    LZ4_UnpackToWRAM((void *)&data_ui_fixed_2bpp_lz4, (void *)(0x007f0000 | ((uint32_t)TILEDATA_ADDR_GAME_UI_2BPP << 1))); 

    DmaSystem_CopyToVram(((uint8_t *)(0x007f0000 | ((uint32_t)TILEDATA_ADDR_GAME_UI_2BPP << 1))), TILEDATA_ADDR_GAME_UI_2BPP, 8192);

    DmaSystem_ResetQueue();

    UserInterface_ClearWindowBuffer(false);
    UserInterface_ClearTextBuffer();

    char temp_error_string[512] = "";

    snprintf((char *)&temp_error_string, 512, (char *)&STR_CRASH_FORMATSTR, 
    crashhandler_a, crashhandler_x, crashhandler_y, crashhandler_flags, (uint8_t)crashhandler_emulation_mode, 
    crashhandler_sp, crashhandler_directpage, crashhandler_pc, crashhandler_databank, 
    crashhandler_regs[0], crashhandler_regs[1],
    crashhandler_regs[2], crashhandler_regs[3], 
    crashhandler_regs[4], crashhandler_regs[5], 
    crashhandler_regs[6], crashhandler_regs[7], 
    crashhandler_regs[8], crashhandler_regs[9], 
    crashhandler_regs[10], crashhandler_regs[11], 
    crashhandler_regs[12], crashhandler_regs[13], 
    crashhandler_regs[14], crashhandler_regs[15],

    crashhandler_regs_float[0], crashhandler_regs_float[1], 
    crashhandler_regs_float[2], crashhandler_regs_float[3],

    crashhandler_stack[0], crashhandler_stack[1],
    crashhandler_stack[2], crashhandler_stack[3], 
    crashhandler_stack[4], crashhandler_stack[5], 
    crashhandler_stack[6], crashhandler_stack[7],
    crashhandler_stack[8], crashhandler_stack[9],
    crashhandler_stack[10], crashhandler_stack[11], 
    crashhandler_stack[12], crashhandler_stack[13], 
    crashhandler_stack[14], crashhandler_stack[15]);

    UserInterface_DrawWindowText((char *)&temp_error_string, 2, 2);

    UserInterface_CopyUiBuffers();
    
    DmaSystem_ProcessQueue();

    DmaSystem_UploadCgram();

    REG_INIDISP = 0x0f;

    return;
}
