#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "spr.h"
#include "spr_metaspr.h"

// Draws a metasprite
#if VBCC_ASM == 1
NO_INLINE void SpriteEngine_AddMetaSprite(__reg("a/x") struct game_object * o, __reg("r0/r1") const struct spr_metaspr_definition * m)
#else
void SpriteEngine_AddMetaSprite(struct game_object * o, const struct spr_metaspr_definition * m)
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\tphy\n"

            "\tpei (r0)\n" // game object's address; r1 is untouched
            
            "\tpei (r2)\n" // holds sprite queue Y index
            "\tpei (r3)\n" // holds depth
            "\tpei (r4)\n" // holds origin X pos
            "\tpei (r5)\n" // holds origin Y pos

            "\ttax\n"
            
            // Precalculate the origin and depth
            "\tlda $7e000c,x\n"
            "\tsec\n"
            "\tsbc <_bg_scroll_y+2\n"
            "\ttay\n"
            "\tsec\n"
            "\tsbc $7e0010,x\n"
            "\tsta r5\n"
            "\ttya\n"
            "\tclc\n"
            "\tadc #16\n"
            "\tand #$00ff\n"
            "\tsta r3\n"

            "\tlda $7e0008,x\n" // carry is guaranteed clear
            "\tsec\n"
            "\tsbc <_bg_scroll_x+2\n"
            "\tsta r4\n"

            ".metasprite_loop:\n"
            /* Metasprite defs
                uint16_t tileattrib; +0
                int16_t offset_x; +2
                int16_t offset_y; +4
                uint16_t size; +6
            */
            // Test if sprite queue full
            "\tlda <_spr_normal_count\n"
            "\tcmp #64\n"
            "\tbcs .finish\n"

            "\tasl\n"
            "\tasl\n"
            "\tasl\n"
            "\tasl\n" // clears carry
            "\tsta r2\n"

            // Test size
            "\tldy #6\n"
            "\tlda [r0],y\n"

            "\tbmi .finish\n"
            "\tbne .large\n"

            ".small:\n"
                "\tlda r4\n"
                "\tldy #2\n"
                "\tadc [r0],y\n"
                
                "\tbpl .x_pos\n"
                ".x_neg:\n"
                    "\tldy r2\n"
                    "\tcmp #-16\n"
                    "\tbcc .next_item\n"
                    // Object partially on the left edge
                    "\tsta _spr_queue_normal,y\n"
                    "\tlda #$40\n"
                    "\tsta _spr_queue_normal+6,y\n"
                    "\tclc\n"
                    "\tbra .y_test\n"
                ".x_pos:\n"
                    "\tldy r2\n"
                    "\tcmp #256\n"
                    "\tbcs .next_item\n"
                    "\tsta _spr_queue_normal,y\n"
                    "\tlda #$00\n"
                    "\tsta _spr_queue_normal+6,y\n"
                ".y_test:\n"

                "\tlda r5\n"
                "\tldy #4\n"
                "\tadc [r0],y\n"
                
                "\tbpl .y_pos\n"
                ".y_neg:\n"
                    "\tcmp #-16\n"
                    "\tbcc .next_item\n"
                    "\tbra .draw\n"
                ".y_pos:\n"
                    "\tcmp #224\n"
                    "\tbcs .next_item\n"
                    "\tbra .draw\n"

            ".large:\n"
                "\tlda r4\n"
                "\tldy #2\n"
                "\tadc [r0],y\n"
                
                "\tbpl .x_pos_lg\n"
                ".x_neg_lg:\n"
                    "\tcmp #-32\n"
                    "\tbcc .next_item\n"
                    // Object partially on the left edge
                    "\tldy r2\n"
                    "\tsta _spr_queue_normal,y\n"
                    "\tlda #$c0\n"
                    "\tsta _spr_queue_normal+6,y\n"
                    "\tclc\n"
                    "\tbra .y_test_lg\n"
                ".x_pos_lg:\n"
                    "\tcmp #256\n"
                    "\tbcs .next_item\n"
                    "\tldy r2\n"
                    "\tsta _spr_queue_normal,y\n"
                    "\tlda #$80\n"
                    "\tsta _spr_queue_normal+6,y\n"
                ".y_test_lg:\n"
                "\tlda r5\n"
                "\tldy #4\n"
                "\tadc [r0],y\n"

                "\tbpl .y_pos_lg\n"

                ".y_neg_lg:\n"
                    "\tcmp #-32\n"
                    "\tbcc .next_item\n"
                    "\tbra .draw\n"
                ".y_pos_lg:\n"
                    "\tcmp #224\n"
                    "\tbcs .next_item\n"

                ".draw:\n"
                    "\tldy r2\n"
                    "\tsta _spr_queue_normal+2,y\n"
                    "\tlda r3\n"
                    "\tsta _spr_queue_normal+8,y\n"
                    "\tlda [r0]\n"
                    "\tsta _spr_queue_normal+4,y\n"
                    "\tinc <_spr_normal_count\n"
            ".next_item:\n"

            "\tlda r0\n"
            "\tclc\n"
            "\tadc #8\n"
            "\tsta r0\n"
            "\tbra .metasprite_loop\n"

            ".finish:\n"

            "\tply\n"
            "\tsty r5\n"
            "\tply\n"
            "\tsty r4\n"
            "\tply\n"
            "\tsty r3\n"
            "\tply\n"
            "\tsty r2\n"

            "\tply\n"
            "\tsty r0\n"

            "\tply\n"
        );
    #else
        int16_t temp_x;
        int16_t temp_y;
        uint16_t temp_depth;

        temp_depth = ((o->pos.y.lh.h + 15) - bg_scroll_y.full.high.a) & 0x00ff;

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
    #endif

    return;
}
