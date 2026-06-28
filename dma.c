#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "dma.h"

NEAR struct dma_entry dma_queue[DMA_QUEUE_MAX_ENTRIES];
uint16_t dma_queue_count;
uint16_t dma_queue_length;

bool dma_filler_enable;
uint16_t dma_filler_dest;
uint16_t dma_filler_length;

/*
    Clears a block of WRAM.
*/
void DmaSystem_ClearWram(
    uint32_t dest, 
    uint16_t length)
{
    REG_DMAP7 = 0x08; // byte reg write, fixed

    REG_A1T7LH = (uint16_t)((uint32_t)&const_zero);
    REG_A1B7 = (uint8_t)((uint32_t)&const_zero >> 16);

    REG_BBAD7 = 0x80; // WMDATA

    REG_WMADDLM = (uint16_t)dest;
    REG_WMADDH = (uint8_t)((uint32_t)dest >> 16);
    
    REG_DAS7LH = length;

    REG_MDMAEN = 0x80;

    return;
}

/*
    Performs a block move using DMA.

    Length of 0 = 65,536 bytes.

    Won't work if source is internal WRAM.

    Uses the reserved for main loop channels (i.e. 7) so it can be called anytime
    though if HDMA is enabled care should be taken to avoid
    HDMA-DMA collision crashes
*/
#if VBCC_ASM == 1
    NO_INLINE void DmaSystem_CopyToWram(
    __reg("r0/r1") uint32_t src, 
    __reg("r2/r3") uint32_t dest, 
    __reg("a") uint16_t length)
#else
void DmaSystem_CopyToWram(
    uint32_t src, 
    uint32_t dest, 
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

        REG_A1T7LH = (uint16_t)src;
        REG_A1B7 = (uint8_t)((uint32_t)src >> 16);

        REG_BBAD7 = 0x80; // WMDATA

        REG_WMADDLM = (uint16_t)dest;
        REG_WMADDH = (uint8_t)((uint32_t)dest >> 16);
        
        REG_DAS7LH = length;

        REG_MDMAEN = 0x80;
    #endif

    return;
}

/*
    Helper functions for doing small DMA runs to minimize overhead. It's still faster than MVN!
*/

// Call this before performing any short runs
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
    NO_INLINE void DmaSystem_CopyToWram_ShortRun(
    __reg("r0") uint16_t src, 
    __reg("x") uint16_t dest, 
    __reg("a") uint16_t length)
#else
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

void DmaSystem_CopyToVram(
    uint32_t src, 
    uint16_t dest, 
    uint16_t length)
{
    // Copies A-bus address to VRAM
    // Must be used during fblank
    // Used for bulk transfers and bypasses the queue.

    REG_DMAP0 = 0x01; // word reg write

    REG_A1T0LH = (uint16_t)src;
    REG_A1B0 = (uint8_t)((uint32_t)src >> 16);

    REG_VMAIN = VRAM_INCHIGH;

    REG_BBAD0 = 0x18; // VMDATAL

    REG_VMADDLH = dest;
    
    REG_DAS0LH = length;

    REG_MDMAEN = 0x01;

    return;
}

void DmaSystem_CopyFromVramToWram(
    uint16_t src, 
    uint32_t dest, 
    uint16_t length)
{
    // Copies VRAM to A-bus address
    // Must be used during fblank
    // Used for bulk transfers and bypasses the queue.

    REG_DMAP0 = 0x81; // word reg write, reverse

    REG_A1T0LH = (uint16_t)dest;
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

/*
    Update CGRAM from shadow for only a given range

    Useful for "additional CPU frame" since HDMA palette changes stick until a full refresh.

    start = the CGRAM palette entry to refresh
    length = amount of entries
*/
#if VBCC_ASM == 1
    NO_INLINE void DmaSystem_UploadCgram_Subset(uint16_t start, uint16_t len)
#else
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

/*
    Dedicated routine for copying water tile animations during
    odd-frame NMI.
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

/*
    Dedicated routine for copying full height (32px) background rows during
    odd-frame NMI.
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

/*
    Split is 1 << split count
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
    
    if (system_fblank_enabled)
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

/*
    Call to reset the queue (throw away everything inside).

    Useful when changing from a major loop to another to prevent VRAM corruption.
*/
void DmaSystem_ResetQueue()
{
    dma_queue_count = 0;
    dma_queue_length = 0;
    dma_filler_enable = false;

    return;
}

uint16_t DmaSystem_SetClear(uint16_t dest, uint16_t length)
{
    // Check for capacity (count, length) issues
    uint16_t temp_length = length + const_lut_dma_split_lookup[0] + dma_queue_length;

    if (temp_length > DMA_QUEUE_MAX_LENGTH)
    {
        return 1; // out of DMA bandwidth
    }

    dma_filler_dest = dest;
    dma_filler_length = length;
    dma_filler_enable = true;

    return 0;
}

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

        REG_A1T0LH = (uint16_t)((uint32_t)&const_zero);
        REG_A1B0 = (uint8_t)(((uint32_t)&const_zero) >> 16);

        REG_DAS0LH = dma_filler_length;

        REG_MDMAEN = 0x01;

        dma_filler_enable = false;
    }

    return;
}
