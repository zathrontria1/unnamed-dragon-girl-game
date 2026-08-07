#include <stdint.h>

#include "vars.h"

#include "spr.h"
#include "spr_metaspr.h"

#include "map.h"

#if VBCC_ASM == 0

void SpriteEngine_AddMetaSprite(struct game_object * o, const struct spr_metaspr_definition * m)
{
    int16_t temp_x;
    int16_t temp_y;
    int16_t temp_depth_signed = o->pos.y.lh.h + 16 - bg_scroll_y.full.high.a;

    if (temp_depth_signed > 255)
    {
        temp_depth_signed = 255;
    }
    else if (temp_depth_signed < 0)
    {
        temp_depth_signed = 0;
    }

    uint16_t temp_depth = temp_depth_signed & 0x00ff;

    while (m->size != 0xffff)
    {
        if (spr_normal_count >= SPR_COUNT_MAX_SORTED)
        {
            break;
        }

        temp_x = o->pos.x.lh.h + m->offset_x - bg_scroll_x.full.high.a;

        if (m->size == 0) // 16px sprite
        {
            if ((temp_x > -16) && (temp_x < 256))
            {
                if (temp_x < 0)
                {
                    spr_queue_normal[spr_normal_count].signsize = 0x40;
                }
                else
                {
                    spr_queue_normal[spr_normal_count].signsize = 0x00;
                }

                temp_y = o->pos.y.lh.h + m->offset_y - o->pos.z.lh.h - bg_scroll_y.full.high.a;

                if ((temp_y > -16) && (temp_y < 224))
                {
                    spr_queue_normal[spr_normal_count].x = temp_x;
                    spr_queue_normal[spr_normal_count].y = temp_y;
                    spr_queue_normal[spr_normal_count].tileattrib = m->tileattrib;
                    spr_queue_normal[spr_normal_count].depth = temp_depth;
                    spr_normal_count++;
                }
            }
        }
        else
        {
            if ((temp_x > -32) && (temp_x < 256))
            {
                if (temp_x < 0)
                {
                    spr_queue_normal[spr_normal_count].signsize = 0xc0;
                }
                else
                {
                    spr_queue_normal[spr_normal_count].signsize = 0x80;
                }

                temp_y = o->pos.y.lh.h + m->offset_y - o->pos.z.lh.h - bg_scroll_y.full.high.a;

                if ((temp_y > -32) && (temp_y < 224))
                {
                    spr_queue_normal[spr_normal_count].x = temp_x;
                    spr_queue_normal[spr_normal_count].y = temp_y;
                    spr_queue_normal[spr_normal_count].tileattrib = m->tileattrib;
                    spr_queue_normal[spr_normal_count].depth = temp_depth;
                    spr_normal_count++;
                }
            }
        }

        m++;
    }

    return;
}

void SpriteEngine_AddMetaSprite_Back(struct game_object * o, const struct spr_metaspr_definition * m)
{
    int16_t temp_x;
    int16_t temp_y;
    int16_t temp_depth_signed = o->pos.y.lh.h + 16 - bg_scroll_y.full.high.a;

    if (temp_depth_signed > 255)
    {
        temp_depth_signed = 255;
    }
    else if (temp_depth_signed < 0)
    {
        temp_depth_signed = 0;
    }

    uint16_t temp_depth = temp_depth_signed & 0x00ff;

    while (m->size != 0xffff)
    {
        if (spr_back_count >= SPR_COUNT_MAX_SORTED)
        {
            break;
        }

        temp_x = o->pos.x.lh.h + m->offset_x - bg_scroll_x.full.high.a;

        if (m->size == 0) // 16px sprite
        {
            if ((temp_x > -16) && (temp_x < 256))
            {
                if (temp_x < 0)
                {
                    spr_queue_back[spr_back_count].signsize = 0x40;
                }
                else
                {
                    spr_queue_back[spr_back_count].signsize = 0x00;
                }

                temp_y = o->pos.y.lh.h + m->offset_y - o->pos.z.lh.h - bg_scroll_y.full.high.a;

                if ((temp_y > -16) && (temp_y < 224))
                {
                    spr_queue_back[spr_back_count].x = temp_x;
                    spr_queue_back[spr_back_count].y = temp_y;
                    spr_queue_back[spr_back_count].tileattrib = m->tileattrib;
                    spr_queue_back[spr_back_count].depth = temp_depth;
                    spr_back_count++;
                }
            }
        }
        else
        {
            if ((temp_x > -32) && (temp_x < 256))
            {
                if (temp_x < 0)
                {
                    spr_queue_back[spr_back_count].signsize = 0xc0;
                }
                else
                {
                    spr_queue_back[spr_back_count].signsize = 0x80;
                }

                temp_y = o->pos.y.lh.h + m->offset_y - o->pos.z.lh.h - bg_scroll_y.full.high.a;

                if ((temp_y > -32) && (temp_y < 224))
                {
                    spr_queue_back[spr_back_count].x = temp_x;
                    spr_queue_back[spr_back_count].y = temp_y;
                    spr_queue_back[spr_back_count].tileattrib = m->tileattrib;
                    spr_queue_back[spr_back_count].depth = temp_depth;
                    spr_back_count++;
                }
            }
        }

        m++;
    }

    return;
}

#endif
