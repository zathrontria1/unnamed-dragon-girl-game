#include <stdint.h>

#include "vars.h"

#include "spr.h"
#include "spr_metaspr.h"

// Draws a metasprite
void spr_metaspr_draw(struct game_object * o, const struct spr_metaspr_definition * m)
{
    struct spr_queue_entry s;
    s.depth = (o->pos.y.lh.h + 15) - bg_scroll_y.full.high.a;

    while (m->size != 0xffff)
    {
        if (spr_normal_count >= 128)
        {
            break;
        }
        
        s.x = o->pos.x.lh.h + m->offset_x - bg_scroll_x.full.high.a;

        if (m->size == 0) // 16px sprite
        {
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
                s.y = o->pos.y.lh.h + m->offset_y - bg_scroll_y.full.high.a;

                if ((s.y > -16) && (s.y < 224))
                {
                    s.tileattrib = m->tileattrib;
                    spr_queue_add(&s, &spr_queue_normal[spr_normal_count]);
                    spr_normal_count++;
                }
            }
        }
        else
        {
            if ((s.x > -32) && (s.x < 256))
            {
                if (s.x < 0)
                {
                    s.signsize = 0xc0;
                }
                else
                {
                    s.signsize = 0x80;
                }

                s.y = o->pos.y.lh.h + m->offset_y - bg_scroll_y.full.high.a;

                if ((s.y > -32) && (s.y < 224))
                {
                    s.tileattrib = m->tileattrib;
                    spr_queue_add(&s, &spr_queue_normal[spr_normal_count]);
                    spr_normal_count++;
                }
            }
        }

        m++;
    }

    return;
}
