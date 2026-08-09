#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "spr.h"
#include "map.h"
#include "system.h"
#include "dma.h"

uint16_t spr_sprite_count; // Rendered sprites this frame
uint16_t spr_sprite_count_prev; // previous

uint16_t spr_vram_free_mask[5];
uint16_t spr_vram_owner_slot[256];

uint16_t spr_front_count; // Rendered non-UI unsorted front-forced sprites this frame
NEAR struct spr_queue_entry spr_queue_front[SPR_COUNT_MAX_FRONT];
uint16_t spr_back_count; // Rendered non-UI unsorted back-forced sprites this frame (e.g. background impostors and shadows)
NEAR struct spr_queue_entry spr_queue_back[SPR_COUNT_MAX_BACK];

uint16_t spr_normal_count;
//NEAR uint8_t spr_depth_count[257]; // Count of sprites on each depth line // Declared in ASM
NEAR struct spr_queue_entry spr_queue_normal[SPR_COUNT_MAX_SORTED]; // depth sorted sprite entries

/*
    Adds a sprite to the draw queue

    It is the responsibility of the caller select the right queue and
    to ensure that the written sprite is valid
*/
/**
 * @brief Renders a single UI sprite cell directly.
 *
 * @param x          Screen X coordinate.
 * @param y          Screen Y coordinate.
 * @param tileattrib VRAM tile attribute word (palette, priority, flip).
 */
void SpriteEngine_DrawUISprite(int16_t x, int16_t y, uint16_t tileattrib)
{
    struct spr_queue_entry s;
    s.x = x;

    if ((s.x > -16) && (s.x < 256))
    {
        if (s.x < 0)
        {
            s.signsize = 0x40;
        }
        else
        {
            s.signsize = 0x00;
        }
        s.y = y;

        if ((s.y > -16) && (s.y < 224))
        {
            s.tileattrib = tileattrib;
            SpriteEngine_DrawSprite(&s);

            return;
        }
    }

    return;
}

// Sprite engine queue/list/write/reset low-level routines are extracted to spr_opt.asm and spr_opt.c.

/**
 * @brief Initializes the dynamic VRAM tile slot allocation table.
 */
void SpriteEngine_InitVramSlot()
{
    for (int i = 0; i < 5; i++)
    {
        spr_vram_free_mask[i] = 0xffff;
    }
    spr_vram_owner_slot[0] = 0xffff;
    System_CopyBlock((uint8_t *)&spr_vram_owner_slot[0], ((uint8_t *)&spr_vram_owner_slot[0]) + 1, 511);

    return;
}

/**
 * @brief Allocates a 16x16 sprite tile slot in VRAM.
 *
 * @param i Preferred slot index.
 * @return The allocated VRAM tile word address offset.
 */
uint16_t SpriteEngine_GetVramSlot16(uint16_t i)
{
    for (int word_idx = 0; word_idx < 5; word_idx++)
    {
        uint16_t mask = spr_vram_free_mask[word_idx];
        if (mask != 0)
        {
            uint16_t bit_pos = 0;
            if ((mask & 0x00ff) == 0)
            {
                bit_pos += 8;
                mask >>= 8;
            }
            if ((mask & 0x000f) == 0)
            {
                bit_pos += 4;
                mask >>= 4;
            }
            if ((mask & 0x0003) == 0)
            {
                bit_pos += 2;
                mask >>= 2;
            }
            if ((mask & 0x0001) == 0)
            {
                bit_pos += 1;
            }

            spr_vram_free_mask[word_idx] &= ~(1 << bit_pos);

            uint16_t slot = 48 + (word_idx << 4) + bit_pos;
            if (i < 256)
            {
                spr_vram_owner_slot[i] = slot;
            }

            return slot;
        }
    }

    return 128;
}

/**
 * @brief Allocates a 32x32 sprite tile slot in VRAM.
 *
 * @param i Preferred slot index.
 * @return The allocated VRAM tile word address offset.
 */
uint16_t SpriteEngine_GetVramSlot32(uint16_t i)
{
    for (int word_idx = 0; word_idx < 5; word_idx++)
    {
        uint16_t mask = spr_vram_free_mask[word_idx];
        if (mask != 0)
        {
            uint16_t bit_pos = 16;

            if ((mask & 0x000f) == 0x000f)
            {
                bit_pos = 0;
                spr_vram_free_mask[word_idx] &= ~0x000f;
            }
            else if ((mask & 0x00f0) == 0x00f0)
            {
                bit_pos = 4;
                spr_vram_free_mask[word_idx] &= ~0x00f0;
            }
            else if ((mask & 0x0f00) == 0x0f00)
            {
                bit_pos = 8;
                spr_vram_free_mask[word_idx] &= ~0x0f00;
            }
            else if ((mask & 0xf000) == 0xf000)
            {
                bit_pos = 12;
                spr_vram_free_mask[word_idx] &= ~0xf000;
            }

            if (bit_pos < 16)
            {
                uint16_t slot = 48 + (word_idx << 4) + bit_pos;
                if (i < 256)
                {
                    spr_vram_owner_slot[i] = slot;
                }

                return slot;
            }
        }
    }

    return 128;
}

bool spr_boss_vram_active = false;

/**
 * @brief Releases allocated VRAM sprite tile slots back to the pool.
 *
 * @param i          Starting slot index.
 * @param slot_count Number of slots to free.
 */
void SpriteEngine_ReleaseVramSlot(uint16_t i, uint16_t slot_count)
{
    if (i >= 256)
    {
        return;
    }

    uint16_t slot = spr_vram_owner_slot[i];
    if (slot >= 48 && slot < 128)
    {
        // If boss VRAM is active, any slots >= 64 are permanently reserved by the boss
        // and shouldn't be marked as free in the allocation mask.
        if (!(spr_boss_vram_active && slot >= 64))
        {
            uint16_t offset = slot - 48;
            uint16_t word_idx = offset >> 4;
            uint16_t bit_idx = offset & 0x000f;

            if (slot_count >= 4)
            {
                spr_vram_free_mask[word_idx] |= (0x000f << bit_idx);
            }
            else
            {
                spr_vram_free_mask[word_idx] |= (1 << bit_idx);
            }
        }

        spr_vram_owner_slot[i] = 0xffff;
    }

    return;
}

/**
 * @brief Reserves high-capacity VRAM pages specifically for boss sprite graphics.
 */
void SpriteEngine_GetVramForBoss()
{
    spr_boss_vram_active = true;
    spr_vram_free_mask[1] = 0x0000;
    spr_vram_free_mask[2] = 0x0000;
    spr_vram_free_mask[3] = 0x0000;
    spr_vram_free_mask[4] = 0x0000;

    return;
}

/**
 * @brief Releases reserved boss VRAM sprite pages back to the allocation pool.
 */
void SpriteEngine_ReleaseVramForBoss()
{
    spr_boss_vram_active = false;
    spr_vram_free_mask[1] = 0xffff;
    spr_vram_free_mask[2] = 0xffff;
    spr_vram_free_mask[3] = 0xffff;
    spr_vram_free_mask[4] = 0xffff;

    return;
}

/**
 * @brief Resets all sprite queues and counters for the next frame,
 *     and resets the shadow OAM tables to a clean state.
 */
void SpriteEngine_ResetSpriteLists()
{
    spr_sprite_count_prev = 128;
    spr_sprite_count = 0;
    spr_front_count = 0;
    spr_back_count = 0;
    spr_normal_count = 0;

    SpriteEngine_ProcessSpriteLists();

    return;
}

void SpriteEngine_ProcessSpriteLists_ClearDepthBuffer()
{
    System_Hsync(0);

    REG_DMAP7 = 0x08; // byte reg write, fixed
    REG_A1T7LH = ADDR_LOWORD(&const_zero);
    REG_A1B7 = ADDR_BANK(&const_zero);
    REG_BBAD7 = 0x80; // WMDATA
    REG_WMADDLM = ADDR_LOWORD(spr_depth_count);
    REG_WMADDH = ADDR_BANK(spr_depth_count);
    REG_DAS7LH = 129;
    REG_MDMAEN = 0x80; // Enable DMA Channel 7

    return;
}

/**
 * @brief Main sprite rendering pipeline coordinator.
 *
 * Sorts queued sprite entries and outputs them to the shadow OAM tables.
 */
void SpriteEngine_ProcessSpriteLists()
{
    SpriteEngine_ProcessSpriteLists_WriteFrontSprites();

    SpriteEngine_ProcessSpriteLists_ClearDepthBuffer();
    SpriteEngine_ProcessSpriteLists_TallySprites();
    SpriteEngine_ProcessSpriteLists_CalculateOffsets();
    SpriteEngine_ProcessSpriteLists_WriteSortedSprites();

    SpriteEngine_ProcessSpriteLists_WriteBackSprites();

    SpriteEngine_ResetOam();
    SpriteEngine_PackOamHighTable();

    return;
}
