#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "ani_bg.h"

#include "dma.h"

#if VBCC_ASM == 0
void DmaSystem_CopyToWram(
    uint8_t * src,
    uint8_t * dest,
    uint16_t length)
{
    // Copies A-bus address to WRAM
    REG_DMAP7 = 0x00; // byte reg write

    REG_A1T7LH = ADDR_LOWORD(src);
    REG_A1B7 = ADDR_BANK(src);

    REG_BBAD7 = 0x80; // WMDATA

    REG_WMADDLM = ADDR_LOWORD(dest);
    REG_WMADDH = ADDR_BANK(dest);

    REG_DAS7LH = length;

    REG_MDMAEN = 0x80;

    return;
}

void DmaSystem_CopyToWram_ShortRun(
    uint16_t src,
    uint16_t dest,
    uint16_t length)
{
    REG_A1T7LH = src;

    REG_WMADDLM = dest;

    REG_DAS7LH = length;

    REG_MDMAEN = 0x80;

    return;
}

void DmaSystem_UploadOam()
{
    REG_OAMADDLH = 0; //reset oam address

    REG_DMAP0 = 0x00; //byte reg write

    REG_A1T0LH = ADDR_LOWORD(&shadow_oam);
    REG_A1B0 = ADDR_BANK(&shadow_oam);

    REG_BBAD0 = 0x04; // OAMDATA

    REG_DAS0LH = 544;

    REG_MDMAEN = 0x01;

    return;
}

void DmaSystem_UploadCgram()
{
    REG_CGADD = 0; //reset palette address

    REG_DMAP0 = 0x00; //byte reg write

    REG_A1T0LH = ADDR_LOWORD(&shadow_cgram);
    REG_A1B0 = ADDR_BANK(&shadow_cgram);

    REG_BBAD0 = 0x22; // CGDATA

    REG_DAS0LH = 512;

    REG_MDMAEN = 0x01;

    return;
}

void DmaSystem_UploadCgram_Subset(uint16_t start, uint16_t len)
{
    REG_CGADD = start; //reset palette address

    REG_DMAP0 = 0x00; //byte reg write

    REG_A1T0LH = ADDR_LOWORD(&shadow_cgram) + (start << 1);
    REG_A1B0 = ADDR_BANK(&shadow_cgram);

    REG_BBAD0 = 0x22; // CGDATA

    REG_DAS0LH = len << 1;

    REG_MDMAEN = 0x01;

    return;
}

void DmaSystem_UpdateStripTiles()
{
    // Handle the background tile DMA here specifically
    REG_DMAP0 = 0x01; // word reg write
    REG_BBAD0 = 0x18; // VMDATAL

    REG_VMAIN = 0x80;
    REG_VMADDLH = ani_bg_dest_water;

    REG_A1T0LH = ADDR_LOWORD(ani_bg_addr_water);
    REG_A1B0 = ADDR_BANK(ani_bg_addr_water);

    REG_DAS0LH = 512;

    REG_MDMAEN = 0x01;

    return;
}

void DmaSystem_UpdateFrameTiles()
{
    // Handle the background tile DMA here specifically
    REG_DMAP0 = 0x01; // word reg write
    REG_BBAD0 = 0x18; // VMDATAL

    REG_VMAIN = 0x80;
    REG_VMADDLH = ani_bg_dest_tallbg;

    REG_A1T0LH = ADDR_LOWORD(ani_bg_addr_tallbg);
    REG_A1B0 = ADDR_BANK(ani_bg_addr_tallbg);

    REG_DAS0LH = 2048; // 512 bytes x 4 rows of 8px = 2048

    REG_MDMAEN = 0x01;

    return;
}

void DmaSystem_ProcessQueue()
{
    REG_DMAP0 = 0x01; // word reg write
    REG_BBAD0 = 0x18; // VMDATAL

    for (int i = 0; i < dma_queue_count; i++)
    {
        REG_VMAIN = dma_queue[i].vmain;
        REG_VMADDLH = dma_queue[i].dest;

        REG_A1T0LH = ADDR_LOWORD(dma_queue[i].src);
        REG_A1B0 = ADDR_BANK(dma_queue[i].src);

        REG_DAS0LH = dma_queue[i].length;

        REG_MDMAEN = 0x01;
    }

    dma_queue_count = 0;
    dma_queue_length = 0;

    if (dma_filler_enable)
    {
        REG_DMAP0 = 0x09; // word reg write, fixed increment

        REG_VMAIN = VRAM_INCHIGH;

        REG_VMADDLH = dma_filler_dest;

        REG_A1T0LH = ADDR_LOWORD(&dma_filler_val);
        REG_A1B0 = ADDR_BANK(&dma_filler_val);

        REG_DAS0LH = dma_filler_length;

        REG_MDMAEN = 0x01;

        dma_filler_enable = false;
    }

    return;
}
#endif
