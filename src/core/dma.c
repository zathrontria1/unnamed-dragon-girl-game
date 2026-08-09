#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "ani_bg.h"

#include "dma.h"

NEAR struct dma_entry dma_queue[DMA_QUEUE_MAX_ENTRIES];
uint16_t dma_queue_count;
uint16_t dma_queue_length;

bool dma_filler_enable;
uint16_t dma_filler_dest;
uint16_t dma_filler_length;

uint8_t dma_filler_val;

/**
 * @brief Clears a block of WRAM by writing zeroes using DMA channel 7.
 * 
 * @param dest   Pointer to the destination address in WRAM.
 * @param length The size of the memory block to clear in bytes. A value of 0 is equivalent to 65,536 bytes.
 */
void DmaSystem_ClearWram(
    uint8_t * dest, 
    uint16_t length)
{
    REG_DMAP7 = 0x08; // byte reg write, fixed

    REG_A1T7LH = ADDR_LOWORD(&const_zero);
    REG_A1B7 = ADDR_BANK(&const_zero);

    REG_BBAD7 = 0x80; // WMDATA

    REG_WMADDLM = ADDR_LOWORD(dest);
    REG_WMADDH = ADDR_BANK(dest);
    
    REG_DAS7LH = length;

    REG_MDMAEN = 0x80;

    return;
}

// Extracted to src/core/dma_opt.asm and src/core/dma_opt.c.

/**
 * @brief Prepares DMA registers for multiple fast small WRAM copies.
 * 
 * Sets up DMA parameters (DMAP, BBAD, A1B, and WMADDH) once to reduce overhead 
 * when calling DmaSystem_CopyToWram_ShortRun iteratively.
 * 
 * @param src_bank  Source bank byte.
 * @param dest_bank Destination bank byte.
 */
void DmaSystem_CopyToWram_ShortPrep(
    uint8_t src_bank, 
    uint8_t dest_bank)
{
    REG_DMAP7 = 0x00; // byte reg write

    REG_A1B7 = src_bank;

    REG_BBAD7 = 0x80; // WMDATA

    REG_WMADDH = dest_bank;
    
    return;
}

// Extracted to src/core/dma_opt.asm and src/core/dma_opt.c.

/**
 * @brief Copies a block of memory from ROM/WRAM to VRAM.
 * 
 * Performs an immediate DMA transfer using DMA channel 7.
 * 
 * @param src    Pointer to the source data.
 * @param dest   The destination word address in VRAM.
 * @param length The number of bytes to copy (must be even).
 */
void DmaSystem_CopyToVram(
    uint8_t * src, 
    uint16_t dest, 
    uint16_t length)
{
    // Copies A-bus address to VRAM
    // Must be used during fblank
    // Used for bulk transfers and bypasses the queue.

    REG_DMAP0 = 0x01; // word reg write

    REG_A1T0LH = ADDR_LOWORD(src);
    REG_A1B0 = ADDR_BANK(src);

    REG_VMAIN = VRAM_INCHIGH;

    REG_BBAD0 = 0x18; // VMDATAL

    REG_VMADDLH = dest;
    
    REG_DAS0LH = length;

    REG_MDMAEN = 0x01;

    return;
}

/**
 * @brief Copies a block of memory from VRAM back to WRAM.
 * 
 * Performs an immediate DMA transfer using DMA channel 7.
 * 
 * @param src    The source word address in VRAM.
 * @param dest   Pointer to the destination address in WRAM.
 * @param length The number of bytes to copy.
 */
void DmaSystem_CopyFromVramToWram(
    uint16_t src, 
    uint8_t * dest, 
    uint16_t length)
{
    // Copies VRAM to A-bus address
    // Must be used during fblank
    // Used for bulk transfers and bypasses the queue.

    REG_DMAP0 = 0x81; // word reg write, reverse

    REG_A1T0LH = ADDR_LOWORD(dest);
    REG_A1B0 = ADDR_BANK(dest);

    REG_VMAIN = VRAM_INCHIGH;

    REG_BBAD0 = 0x39; // VMDATAREADL

    REG_VMADDLH = src;
    
    REG_DAS0LH = length;

    // Perform a dummy read first
    register volatile uint16_t temp = REG_VMDATAREADLH;
    
    REG_MDMAEN = 0x01;

    return;
}

// Extracted to src/core/dma_opt.asm and src/core/dma_opt.c.

// (1 << split count) * DMA_QUEUE_OVERHEAD
NEAR const uint16_t const_lut_dma_split_lookup[6] = {
    (1 << 0) * DMA_QUEUE_OVERHEAD, 
    (1 << 1) * DMA_QUEUE_OVERHEAD, 
    (1 << 2) * DMA_QUEUE_OVERHEAD, 
    (1 << 3) * DMA_QUEUE_OVERHEAD, 
    (1 << 4) * DMA_QUEUE_OVERHEAD, 
    (1 << 5) * DMA_QUEUE_OVERHEAD, 
};

// Extracted to src/core/dma_opt.asm and src/core/dma_opt.c.

/**
 * @brief Resets the DMA queue, clearing all pending transfers.
 */
void DmaSystem_ResetQueue()
{
    dma_queue_count = 0;
    dma_queue_length = 0;
    dma_filler_enable = false;

    return;
}

/**
 * @brief Configures a constant value VRAM filler operation.
 * 
 * Enqueues a DMA clear operation using a single byte value.
 * 
 * @param dest   Word address in VRAM.
 * @param length Number of bytes to fill.
 * @param val    The byte value to fill the VRAM space with.
 * @return A status code (0 on success, 1 on queue failure).
 */
uint16_t DmaSystem_SetClear(uint16_t dest, uint16_t length, uint8_t val)
{
    // Check for capacity (count, length) issues
    uint16_t temp_length = length + const_lut_dma_split_lookup[0] + dma_queue_length;

    if (temp_length > DMA_QUEUE_MAX_LENGTH)
    {
        return 1; // out of DMA bandwidth
    }

    dma_filler_dest = dest;
    dma_filler_length = length;
    dma_filler_val = val;
    dma_filler_enable = true;

    return 0;
}

// Extracted to src/core/dma_opt.asm and src/core/dma_opt.c.
