#include <stdint.h>

#include "vars.h"

#include "spr.h"
#include "map.h"

#if VBCC_ASM == 0

void SpriteEngine_AddToFrontLayer(struct game_object * o, uint16_t tileattrib)
{
    if (spr_front_count >= SPR_COUNT_MAX_FRONT)
    {
        return;
    }

    int16_t temp_x = o->pos.x.lh.h - bg_scroll_x.full.high.a;

    if ((temp_x > -16) && (temp_x < 256))
    {
        struct spr_queue_entry *entry = &spr_queue_front[spr_front_count];

        entry->signsize = (temp_x < 0) ? 0x40 : 0x00;

        int16_t temp_y = o->pos.y.lh.h - o->pos.z.lh.h - bg_scroll_y.full.high.a;

        if ((temp_y > -16) && (temp_y < 224))
        {
            entry->tileattrib = tileattrib;
            entry->x = temp_x;
            entry->y = temp_y;
            entry->depth = ((temp_y + 16) >> 1) & 0x007f;

            spr_front_count++;

            return;
        }
    }

    return;
}

void SpriteEngine_AddToSortedLayer(struct game_object * o, uint16_t tileattrib)
{
    if (spr_normal_count >= SPR_COUNT_MAX_SORTED)
    {
        return;
    }

    int16_t temp_x = o->pos.x.lh.h - bg_scroll_x.full.high.a;

    if ((temp_x > -16) && (temp_x < 256))
    {
        struct spr_queue_entry *entry = &spr_queue_normal[spr_normal_count];

        entry->signsize = (temp_x < 0) ? 0x40 : 0x00;

        int16_t temp_y = o->pos.y.lh.h - o->pos.z.lh.h - bg_scroll_y.full.high.a;

        if ((temp_y > -16) && (temp_y < 224))
        {
            entry->x = temp_x;
            entry->y = temp_y;
            entry->tileattrib = tileattrib;
            entry->depth = ((temp_y + 16) >> 1) & 0x007f;

            spr_normal_count++;

            return;
        }
    }

    return;
}

void SpriteEngine_AddToBackLayer(struct game_object * o, uint16_t tileattrib)
{
    if (spr_back_count >= SPR_COUNT_MAX_BACK)
    {
        return;
    }

    int16_t temp_x = o->pos.x.lh.h - bg_scroll_x.full.high.a;

    if ((temp_x > -16) && (temp_x < 256))
    {
        struct spr_queue_entry *entry = &spr_queue_back[spr_back_count];

        entry->signsize = (temp_x < 0) ? 0x40 : 0x00;

        int16_t temp_y = o->pos.y.lh.h - o->pos.z.lh.h - bg_scroll_y.full.high.a;

        if ((temp_y > -16) && (temp_y < 224))
        {
            entry->tileattrib = tileattrib;
            entry->x = temp_x;
            entry->y = temp_y;
            entry->depth = ((temp_y + 16) >> 1) & 0x007f;

            spr_back_count++;

            return;
        }
    }

    return;
}

void SpriteEngine_ProcessSpriteLists_WriteFrontSprites()
{
    struct spr_queue_entry *queue = &spr_queue_front[0];
    uint16_t count = spr_front_count;
    uint16_t idx = spr_sprite_count;
    for (uint16_t i = 0; i < count; i++)
    {
        if (idx >= 128)
        {
            break;
        }
        shadow_oam.entries.shadow_oam_low[idx].tileattrib = queue->tileattrib;
        shadow_oam.entries.shadow_oam_low[idx].x = (uint8_t)queue->x;
        shadow_oam.entries.shadow_oam_low[idx].y = (uint8_t)queue->y;
        shadow_oam.entries.shadow_oam_high[idx].signsize = queue->signsize;
        idx++;
        queue++;
    }

    if (idx > 128)
    {
        idx = 128;
    }
    spr_sprite_count = idx;
    spr_front_count = 0;

    return;
}

void SpriteEngine_ProcessSpriteLists_TallySprites()
{
    for (int i = 0; i < spr_normal_count; i++)
    {
        uint16_t depth = spr_queue_normal[i].depth & 0x007f;
        spr_depth_count[depth + 1]++;
    }

    return;
}

void SpriteEngine_ProcessSpriteLists_CalculateOffsets()
{
    uint16_t temp_offset = spr_sprite_count + spr_normal_count;
    if (temp_offset > 128)
    {
        temp_offset = 128;
    }
    uint16_t t;

    for (int i = 0; i < 129; i++)
    {
        t = temp_offset;

        spr_depth_count[i] = t;

        temp_offset -= spr_depth_count[i + 1];
    }

    return;
}

void SpriteEngine_ProcessSpriteLists_WriteSortedSprites()
{
    struct spr_queue_entry *queue = &spr_queue_normal[0];
    for (int i = 0; i < spr_normal_count; i++)
    {
        uint16_t depth = queue->depth & 0x007f;
        uint16_t index = --spr_depth_count[depth];

        if (index < 128)
        {
            shadow_oam.entries.shadow_oam_low[index].x = (uint8_t)queue->x;
            shadow_oam.entries.shadow_oam_low[index].y = (uint8_t)queue->y;
            shadow_oam.entries.shadow_oam_low[index].tileattrib = queue->tileattrib;
            shadow_oam.entries.shadow_oam_high[index].signsize = queue->signsize;
        }

        queue++;
    }

    spr_sprite_count += spr_normal_count;
    if (spr_sprite_count > 128)
    {
        spr_sprite_count = 128;
    }
    spr_normal_count = 0;

    return;
}

void SpriteEngine_ProcessSpriteLists_WriteBackSprites()
{
    struct spr_queue_entry *queue = &spr_queue_back[0];
    uint16_t count = spr_back_count;
    uint16_t idx = spr_sprite_count;
    for (uint16_t i = 0; i < count; i++)
    {
        if (idx >= 128)
        {
            break;
        }
        shadow_oam.entries.shadow_oam_low[idx].tileattrib = queue->tileattrib;
        shadow_oam.entries.shadow_oam_low[idx].x = (uint8_t)queue->x;
        shadow_oam.entries.shadow_oam_low[idx].y = (uint8_t)queue->y;
        shadow_oam.entries.shadow_oam_high[idx].signsize = queue->signsize;
        idx++;
        queue++;
    }

    if (idx > 128)
    {
        idx = 128;
    }
    spr_sprite_count = idx;
    spr_back_count = 0;

    return;
}

void SpriteEngine_DrawSprite(struct spr_queue_entry * s)
{
    if (spr_sprite_count >= 128)
    {
        return;
    }

    shadow_oam.entries.shadow_oam_low[spr_sprite_count].tileattrib = s->tileattrib;
    shadow_oam.entries.shadow_oam_low[spr_sprite_count].x = (uint8_t)s->x;
    shadow_oam.entries.shadow_oam_low[spr_sprite_count].y = (uint8_t)s->y;

    shadow_oam.entries.shadow_oam_high[spr_sprite_count].signsize = s->signsize;

    spr_sprite_count++;

    return;
}

void SpriteEngine_PackOamHighTable()
{
    int j = 0;

    for (int i = 0; i < 32; i++)
    {
        shadow_oam.bytes[512+i] =
            (shadow_oam.entries.shadow_oam_high[j].signsize >> 6) |
            (shadow_oam.entries.shadow_oam_high[j+1].signsize >> 4) |
            (shadow_oam.entries.shadow_oam_high[j+2].signsize >> 2) |
            (shadow_oam.entries.shadow_oam_high[j+3].signsize);
        j += 4;
    }

    spr_sprite_count = 0;

    return;
}

void SpriteEngine_ResetOam()
{
    uint16_t temp_len = (spr_sprite_count_prev + 3) & ~0x03;
    if (temp_len > 128)
    {
        temp_len = 128;
    }

    uint16_t start_idx = spr_sprite_count;
    if (start_idx > 128)
    {
        start_idx = 128;
    }

    for (int i = start_idx; i < temp_len; i++)
    {
        // from the first unused entry to the end of previous frame's active entry.
        shadow_oam.entries.shadow_oam_low[i].y = 240;
    }

    for (int i = start_idx; i < 128; i++)
    {
        shadow_oam.entries.shadow_oam_high[i].signsize = 0;
    }

    if (spr_sprite_count > 128)
    {
        spr_sprite_count = 128;
    }
    spr_sprite_count_prev = spr_sprite_count;

    // Do not reset sprite count of current frame yet, the high oam packing needs it
    return;
}

#endif
