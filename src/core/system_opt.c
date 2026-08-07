#include "snes/console.h"

#include <stdint.h>

#include "vars.h"

#include "asm.h"
#include "system.h"

#if VBCC_ASM == 0
void System_GetInput()
{
    // Check if input is ready.
    while ((REG_HVBJOY & PAD_BUSY) == PAD_BUSY)
    {
        ;
    }

    // Store last frame's input temporarily.
    uint16_t temp_pad0 = input_pad0;

    // Current frame input
    input_pad0 = REG_JOYxLH(0) | REG_JOYxLH(1); // Read both and merge

    // Now to figure out what keys were newly pressed
    // XOR with previous frame,
    // then AND with current frame
    input_pad0_new = ((temp_pad0 ^ input_pad0) & input_pad0);

    return;
}

void System_GetInput_Manual()
{
    // Reset controllers
    REG_JOYOUT = 0x01;
    REG_JOYOUT = 0x00;

    // The controller is ready to be read.
    uint16_t controller_bits_0 = 0x0000;
    uint16_t controller_bits_1 = 0x0000;
    uint8_t bit = 0x00; // Read and paste controller data here.

    // Controller 1
    for (int i = 0; i < 16; i++)
    {
        bit = REG_JOYSERx(0);
        bit = bit & 0x01;

        controller_bits_0 = (controller_bits_0 << 1) | bit;
    }

    // Controller 2
    for (int i = 0; i < 16; i++)
    {
        bit = REG_JOYSERx(1);
        bit = bit & 0x01;

        controller_bits_1 = (controller_bits_1 << 1) | bit;
    }

    uint16_t temp_pad0 = input_pad0;
    input_pad0 = controller_bits_0 | controller_bits_1;
    input_pad0_new = ((temp_pad0 ^ input_pad0) & input_pad0);

    return;
}

void System_AlignToVblank()
{
    while ((REG_HVBJOY & VBL_READY) == VBL_READY)
    {
        ;
    }
    while ((REG_HVBJOY & VBL_READY) != VBL_READY)
    {
        ;
    }

    return;
}

void System_Hsync(uint16_t dot)
{
    Asm_EmitSei();
    REG_HTIMELH = dot;
    register volatile uint8_t temp = REG_RDNMI;
    temp = REG_TIMEUP;

    REG_NMITIMEN = shadow_nmitimen | INT_HVIRQ_H;

    Asm_EmitWai();
    temp = REG_RDNMI;
    temp = REG_TIMEUP;

    REG_NMITIMEN = shadow_nmitimen;
    Asm_EmitCli();

    return;
}

void System_CheckForActiveDisplayEnd()
{
    // Check if the current scanline is exactly 224 or not.
    register volatile uint8_t temp = REG_SLHV;
    uint8_t scanline_lo = REG_OPVCT;
    uint8_t scanline_hi = REG_OPVCT & 0x01;
    uint16_t scanline = scanline_lo | (scanline_hi << 8);

    uint16_t target_line = 224; // Last line of active display

    if (system_use_long_vblank)
    {
        target_line = 208;
    }

    if (scanline == target_line)
    {
        while ((scanline == target_line) || (scanline == (target_line + 1)))
        {
            temp = REG_STAT78;
            temp = REG_SLHV;
            scanline_lo = REG_OPVCT;
            scanline_hi = REG_OPVCT & 0x01;
            scanline = scanline_lo | (scanline_hi << 8);
        }
    }

    temp = REG_STAT78;

    return;
}

void System_CopyBlock(uint8_t * src, uint8_t * dest, uint16_t len)
{
    // Source and destination bank independent, just can't cross banks
    for (uint16_t i = 0; i < len; i++)
    {
        (*dest++) = (*src++);
    }

    return;
}
#endif
