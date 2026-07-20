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

    REG_A1T7LH = (uint16_t)((uint32_t)&const_zero);
    REG_A1B7 = (uint8_t)((uint32_t)&const_zero >> 16);

    REG_BBAD7 = 0x80; // WMDATA

    REG_WMADDLM = (uint16_t)((uint32_t)dest);
    REG_WMADDH = (uint8_t)((uint32_t)dest >> 16);
    
    REG_DAS7LH = length;

    REG_MDMAEN = 0x80;

    return;
}

#if VBCC_ASM == 1
/**
 * @brief Copies a block of memory to WRAM using DMA channel 7.
 * 
 * @param src    [r0/r1] Pointer to the source data in ROM or WRAM.
 * @param dest   [r2/r3] Pointer to the destination address in WRAM.
 * @param length [a] The number of bytes to copy. A value of 0 is equivalent to 65,536 bytes.
 */
    NO_INLINE void DmaSystem_CopyToWram(
    __reg("r0/r1") uint8_t * src, 
    __reg("r2/r3") uint8_t * dest, 
    __reg("a") uint16_t length)
#else
/**
 * @brief Copies a block of memory to WRAM using DMA channel 7.
 * 
 * @param src    Pointer to the source data in ROM or WRAM.
 * @param dest   Pointer to the destination address in WRAM.
 * @param length The number of bytes to copy. A value of 0 is equivalent to 65,536 bytes.
 */
void DmaSystem_CopyToWram(
    uint8_t * src, 
    uint8_t * dest, 
    uint16_t length)
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\tsta $4375\n"

            "\tlda r0\n"
            "\tsta $4372\n"

            "\tlda r2\n"
            "\tsta $2181\n"

            "\tlda #$8000\n"
            "\tsta $4370\n"

            "\ta8\n"
            "\tsep #$20\n"
            
            "\tlda r1\n"
            "\tsta $4374\n"

            "\tlda r3\n"
            "\tsta $2183\n"

            "\tlda #$80\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        // Copies A-bus address to WRAM
        REG_DMAP7 = 0x00; // byte reg write

        REG_A1T7LH = (uint16_t)((uint32_t)src);
        REG_A1B7 = (uint8_t)((uint32_t)src >> 16);

        REG_BBAD7 = 0x80; // WMDATA

        REG_WMADDLM = (uint16_t)((uint32_t)dest);
        REG_WMADDH = (uint8_t)((uint32_t)dest >> 16);
        
        REG_DAS7LH = length;

        REG_MDMAEN = 0x80;
    #endif

    return;
}

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

#if VBCC_ASM == 1
/**
 * @brief Executes a rapid small WRAM copy using pre-configured DMA parameters.
 * 
 * @param src    [r0] Source offset address (16-bit) within the prepared bank.
 * @param dest   [x] Destination offset address (16-bit) within the prepared bank.
 * @param length [a] The number of bytes to copy.
 */
    NO_INLINE void DmaSystem_CopyToWram_ShortRun(
    __reg("r0") uint16_t src, 
    __reg("x") uint16_t dest, 
    __reg("a") uint16_t length)
#else
/**
 * @brief Executes a rapid small WRAM copy using pre-configured DMA parameters.
 * 
 * @param src    Source offset address (16-bit) within the prepared bank.
 * @param dest   Destination offset address (16-bit) within the prepared bank.
 * @param length The number of bytes to copy.
 */
void DmaSystem_CopyToWram_ShortRun(
    uint16_t src, 
    uint16_t dest, 
    uint16_t length)
#endif
{

    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\tsta $4375\n"

            "\tlda r0\n"
            "\tsta $4372\n"

            "\tstx $2181\n"

            "\ta8\n"
            "\tsep #$20\n"

            "\tlda #$80\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        REG_A1T7LH = src;

        REG_WMADDLM = dest;
        
        REG_DAS7LH = length;

        REG_MDMAEN = 0x80;
    #endif

    return;
}

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

    REG_A1T0LH = (uint16_t)((uint32_t)src);
    REG_A1B0 = (uint8_t)((uint32_t)src >> 16);

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

    REG_A1T0LH = (uint16_t)((uint32_t)dest);
    REG_A1B0 = (uint8_t)((uint32_t)dest >> 16);

    REG_VMAIN = VRAM_INCHIGH;

    REG_BBAD0 = 0x39; // VMDATAREADL

    REG_VMADDLH = src;
    
    REG_DAS0LH = length;

    // Perform a dummy read first
    register volatile uint16_t temp = REG_VMDATAREADLH;
    
    REG_MDMAEN = 0x01;

    return;
}

/**
 * @brief DMA uploads the shadow OAM buffers to the SNES internal OAM registers.
 * 
 * Copies the low and high OAM tables during VBlank or forced blank.
 */
void DmaSystem_UploadOam()
{
    // Update OAM from shadow
    #if VBCC_ASM == 1
        __asm(
            "\ta8\n"
            "\tsep #$20\n"

            "\tldx #0\n"
            "\tstx $2102\n"

            "\tstz $4300\n"
            
            "\tldx #<_shadow_oam\n"
            "\tstx $4302\n"
            "\tlda #^_shadow_oam\n"
            "\tsta $4304\n"

            "\tldx #544\n"
            "\tstx $4305\n"

            "\tlda #$04\n"
            "\tsta $4301\n"

            "\tlda #1\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        REG_OAMADDLH = 0; //reset oam address

        REG_DMAP0 = 0x00; //byte reg write

        REG_A1T0LH = (uint16_t)(((uint32_t)&shadow_oam));
        REG_A1B0 = (uint8_t)(((uint32_t)&shadow_oam >> 16));
        
        REG_BBAD0 = 0x04; // OAMDATA

        REG_DAS0LH = 544;

        REG_MDMAEN = 0x01;
    #endif

    return;
}

/**
 * @brief DMA uploads the entire palette buffer to CGRAM.
 */
void DmaSystem_UploadCgram()
{
    // Update CGRAM from shadow
    #if VBCC_ASM == 1
        __asm(
            "\ta8\n"
            "\tsep #$20\n"

            "\tstz $2121\n"
            "\tstz $4300\n"
            
            "\tldx #<_shadow_cgram\n"
            "\tstx $4302\n"
            "\tlda #^_shadow_cgram\n"
            "\tsta $4304\n"

            "\tldx #512\n"
            "\tstx $4305\n"

            "\tlda #$22\n"
            "\tsta $4301\n"

            "\tlda #1\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        REG_CGADD = 0; //reset palette address

        REG_DMAP0 = 0x00; //byte reg write

        REG_A1T0LH = (uint16_t)(((uint32_t)&shadow_cgram));
        REG_A1B0 = (uint8_t)(((uint32_t)&shadow_cgram >> 16));

        REG_BBAD0 = 0x22; // CGDATA

        REG_DAS0LH = 512;

        REG_MDMAEN = 0x01;
    #endif

    return;
}

#if VBCC_ASM == 1
/**
 * @brief DMA uploads a subset of the palette buffer to CGRAM.
 * 
 * @param start [r0] The starting index of the palette entry.
 * @param len   [a] The number of CGRAM color entries (words) to upload.
 */
    NO_INLINE void DmaSystem_UploadCgram_Subset(uint16_t start, uint16_t len)
#else
/**
 * @brief DMA uploads a subset of the palette buffer to CGRAM.
 * 
 * @param start The starting index of the palette entry.
 * @param len   The number of CGRAM color entries (words) to upload.
 */
    void DmaSystem_UploadCgram_Subset(uint16_t start, uint16_t len)
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\ta8\n"
            "\tsep #$20\n"

            "\tsta $2121\n"
            "\tstz $4300\n"

            "\ta16\n"
            "\trep #$21\n"

            "\tasl\n"
            "\tadc #<_shadow_cgram\n"
            "\tsta $4302\n"

            "\tlda 4,s\n"
            "\tasl\n"
            "\tsta $4305\n"

            "\ta8\n"
            "\tsep #$20\n"

            "\tlda #^_shadow_cgram\n"
            "\tsta $4304\n"

            "\tlda #$22\n"
            "\tsta $4301\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        REG_CGADD = start; //reset palette address

        REG_DMAP0 = 0x00; //byte reg write

        REG_A1T0LH = (uint16_t)(((uint32_t)&shadow_cgram)) + (start << 1);
        REG_A1B0 = (uint8_t)(((uint32_t)&shadow_cgram >> 16));

        REG_BBAD0 = 0x22; // CGDATA

        REG_DAS0LH = len << 1;

        REG_MDMAEN = 0x01;
    #endif

    return;
}

/**
 * @brief Performs DMA transfers for background tile data that is intended to be updated on a per-row basis (e.g. water animations)
 */
void DmaSystem_UpdateStripTiles()
{
    #if VBCC_ASM == 1
        __asm(
            "\ta8\n"
            "\tsep #$20\n"

            "\tlda #$01\n"
            "\tsta $4300\n"
            "\tlda #$18\n"
            "\tsta $4301\n"

            "\tlda #$80\n"
            "\tsta $2115\n"

            "\tldx _ani_bg_dest_water\n"
            "\tstx $2116\n"

            "\tldx _ani_bg_addr_water\n"
            "\tstx $4302\n"

            "\tlda _ani_bg_addr_water+2\n"
            "\tsta $4304\n"

            "\tldx #512\n"
            "\tstx $4305\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        // Handle the background tile DMA here specifically
        REG_DMAP0 = 0x01; // word reg write
        REG_BBAD0 = 0x18; // VMDATAL

        REG_VMAIN = 0x80;
        REG_VMADDLH = ani_bg_dest_water;

        REG_A1T0LH = (uint16_t)((uint32_t)ani_bg_addr_water);
        REG_A1B0 = (uint8_t)(((uint32_t)ani_bg_addr_water) >> 16);

        REG_DAS0LH = 512;

        REG_MDMAEN = 0x01;
    #endif

    return;
}

/**
 * @brief Performs DMA transfers for background tile data that makes up a 64px background row (e.g. larger background "object" tiles).
 */
void DmaSystem_UpdateFrameTiles()
{
    #if VBCC_ASM == 1
        __asm(
            "\ta8\n"
            "\tsep #$20\n"

            "\tlda #$01\n"
            "\tsta $4300\n"
            "\tlda #$18\n"
            "\tsta $4301\n"

            "\tlda #$80\n"
            "\tsta $2115\n"

            "\tldx _ani_bg_dest_tallbg\n"
            "\tstx $2116\n"

            "\tldx _ani_bg_addr_tallbg\n"
            "\tstx $4302\n"

            "\tlda _ani_bg_addr_tallbg+2\n"
            "\tsta $4304\n"

            "\tldx #2048\n"
            "\tstx $4305\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        // Handle the background tile DMA here specifically
        REG_DMAP0 = 0x01; // word reg write
        REG_BBAD0 = 0x18; // VMDATAL

        REG_VMAIN = 0x80;
        REG_VMADDLH = ani_bg_dest_tallbg;

        REG_A1T0LH = (uint16_t)((uint32_t)ani_bg_addr_tallbg);
        REG_A1B0 = (uint8_t)(((uint32_t)ani_bg_addr_tallbg) >> 16);

        REG_DAS0LH = 2048; // 512 bytes x 4 rows of 8px = 2048

        REG_MDMAEN = 0x01;
    #endif

    return;
}

// (1 << split count) * DMA_QUEUE_OVERHEAD
NEAR const uint16_t const_lut_dma_split_lookup[6] = {
    (1 << 0) * DMA_QUEUE_OVERHEAD, 
    (1 << 1) * DMA_QUEUE_OVERHEAD, 
    (1 << 2) * DMA_QUEUE_OVERHEAD, 
    (1 << 3) * DMA_QUEUE_OVERHEAD, 
    (1 << 4) * DMA_QUEUE_OVERHEAD, 
    (1 << 5) * DMA_QUEUE_OVERHEAD, 
};

/**
 * @brief Enqueues a DMA transfer request to be processed during the next VBlank.
 * 
 * Automatically handles splitting transfers for updates to multiple rows that do not span the entire width (such as sprite tiles arranged in 2D grid).
 * 
 * @param src    Pointer to the source data in ROM/WRAM.
 * @param dest   Word address in VRAM.
 * @param length The total size of the transfer in bytes.
 * @param vmain  VRAM address translation mode (VMAIN register settings).
 * @param split  Split divisor index (used to segment rectangular copies, e.g. 0 to 5).
 * @return A status code: 0 on success, or 1 if the queue is full or bandwidth limit is exceeded.
 */
uint16_t DmaSystem_AddItemToQueue(
    uint8_t * src, 
    uint16_t dest, 
    uint16_t length,
    uint16_t vmain,
    uint16_t split)
{
    // Check for capacity (count, length) issues
    uint16_t temp_length = length + const_lut_dma_split_lookup[split] + dma_queue_length;

    uint16_t temp_queue_count = dma_queue_count + (1 << split);

    if (temp_queue_count > DMA_QUEUE_MAX_ENTRIES)
    {
        return 1; // exceeds current max queue count
    }
    
    if (system_use_long_vblank)
    {
        if (temp_length > DMA_QUEUE_MAX_LENGTH_FBE)
        {
            return 1; // out of DMA bandwidth
        }
    }
    else
    {
        if (temp_length > DMA_QUEUE_MAX_LENGTH)
        {
            return 1; // out of DMA bandwidth
        }
    }

    // Add the valid entry
    if (split > 0)
    {
        // Item is a split transfer
        // For split transfers, assuming a 128px wide cylinder.
        uint16_t split_count = 1 << split;

        uint16_t temp_length_chunk = length >> split;

        for (int i = 0; i < split_count; i++)
        {
            dma_queue[dma_queue_count].vmain = vmain;
            dma_queue[dma_queue_count].src = src;
            dma_queue[dma_queue_count].dest = dest;
            dma_queue[dma_queue_count].length = temp_length_chunk;

            src += 512;
            dest += 256;

            dma_queue_count++;
        }
    }
    else
    {
        // Item is a contiguous transfer
        dma_queue[dma_queue_count].vmain = vmain;
        dma_queue[dma_queue_count].src = src;
        dma_queue[dma_queue_count].dest = dest;
        dma_queue[dma_queue_count].length = length;

        dma_queue_count++;
    }

    dma_queue_length = temp_length;
    
    return 0;
}

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

/**
 * @brief Processes and executes all queued DMA transfers.
 * 
 * Can only be called during VBlank or Fblank. Iterates through the dma_queue and triggers transfers.
 */
void DmaSystem_ProcessQueue() 
{
    #if VBCC_ASM == 1
        __asm(
            "\tsep #$20\n"
            "\ta8\n"
            "\tlda _dma_queue_count\n"
            "\tbeq .dma_queue_empty\n"
            "\tphd\n"
            "\tpea $4300\n"
            "\tpld\n"
            "\tsta $0b\n"

            "\tlda #$01\n"
            "\tsta $00\n"
            "\tlda #$18\n"
            "\tsta $01\n"
            
            "\tphy\n"
            "\tldy #0\n"
        ".loop_dma_queue:\n"
            "\tlda _dma_queue,y\n"
            "\tsta $2115\n"
            "\tldx _dma_queue+2,y\n"
            "\tstx $02\n"
            "\tlda _dma_queue+4,y\n"
            "\tsta $04\n"
            "\tldx _dma_queue+6,y\n"
            "\tstx $2116\n"
            "\tldx _dma_queue+8,y\n"
            "\tstx $05\n"
            "\tlda #1\n"
            "\tsta $420b\n"
            "\ta16\n"
            "\trep #$21\n"
            "\ttya\n"
            "\tadc #16\n"
            "\ttay\n"
            "\tsep #$20\n"
            "\ta8\n"
            "\tdec $0b\n"
            "\tbne .loop_dma_queue\n"
            "\tply\n"
            "\tpld\n"
        ".dma_queue_empty:\n"
            "\trep #$20\n"
            "\ta16\n"
            "\tstz _dma_queue_count\n"
            "\tstz _dma_queue_length\n"
        );
    #else
        REG_DMAP0 = 0x01; // word reg write
        REG_BBAD0 = 0x18; // VMDATAL

        for (int i = 0; i < dma_queue_count; i++) 
        {
            REG_VMAIN = dma_queue[i].vmain;
            REG_VMADDLH = dma_queue[i].dest;

            REG_A1T0LH = (uint16_t)((uint32_t)dma_queue[i].src);
            REG_A1B0 = (uint8_t)(((uint32_t)dma_queue[i].src) >> 16);

            REG_DAS0LH = dma_queue[i].length;

            REG_MDMAEN = 0x01;
        }

        dma_queue_count = 0;
        dma_queue_length = 0;
    #endif

    if (dma_filler_enable)
    {
        REG_DMAP0 = 0x09; // word reg write, fixed increment

        REG_VMAIN = VRAM_INCHIGH;

        REG_VMADDLH = dma_filler_dest;

        REG_A1T0LH = (uint16_t)((uint32_t)&dma_filler_val);
        REG_A1B0 = (uint8_t)(((uint32_t)&dma_filler_val) >> 16);

        REG_DAS0LH = dma_filler_length;

        REG_MDMAEN = 0x01;

        dma_filler_enable = false;
    }

    return;
}
