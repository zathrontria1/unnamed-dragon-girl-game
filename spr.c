#include <stdint.h>

#include "vars.h"

#include "spr.h"

/*
    Adds a sprite to the draw queue

    It is the responsibility of the caller select the right queue and 
    to ensure that the written sprite is valid
*/
void spr_queue_add_ui_wrapper(int16_t x, int16_t y, uint16_t tileattrib)
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
            spr_draw(&s);

            return;
        }
    }

    return;
}

void spr_queue_add_front_wrapper(struct game_object * o, uint16_t tileattrib)
{
    if (spr_front_count >= 128)
    {
        return;
    }

    struct spr_queue_entry s;
    s.x = o->pos.x.lh.h - bg_scroll_x.full.high.a;

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
        s.y = o->pos.y.lh.h - o->pos.z.lh.h - bg_scroll_y.full.high.a;

        if ((s.y > -16) && (s.y < 224))
        {
            s.tileattrib = tileattrib;
            s.depth = (uint8_t)(s.y + 15);
            spr_queue_add(&s, &spr_queue_front[spr_front_count]);
            spr_front_count++;

            return;
        }
    }

    return;
}

void spr_queue_add_normal_wrapper(struct game_object * o, uint16_t tileattrib)
{
    if (spr_normal_count >= 128)
    {
        return;
    }

    struct spr_queue_entry s;
    s.x = o->pos.x.lh.h - bg_scroll_x.full.high.a;

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
        s.y = o->pos.y.lh.h - o->pos.z.lh.h - bg_scroll_y.full.high.a;

        if ((s.y > -16) && (s.y < 224))
        {
            s.tileattrib = tileattrib;
            s.depth = (uint8_t)(s.y + 15);
            spr_queue_add(&s, &spr_queue_normal[spr_normal_count]);
            spr_normal_count++;

            return;
        }
    }

    return;
}

void spr_queue_add_back_wrapper(struct game_object * o, uint16_t tileattrib)
{
    if (spr_back_count >= 128)
    {
        return;
    }

    struct spr_queue_entry s;
    s.x = o->pos.x.lh.h - bg_scroll_x.full.high.a;

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
        s.y = o->pos.y.lh.h - o->pos.z.lh.h - bg_scroll_y.full.high.a;

        if ((s.y > -16) && (s.y < 224))
        {
            s.tileattrib = tileattrib;
            s.depth = (uint8_t)(s.y + 15);
            spr_queue_add(&s, &spr_queue_back[spr_back_count]);
            spr_back_count++;

            return;
        }
    }

    return;
}

#ifdef __VBCC__
    NO_INLINE void spr_queue_add(__reg("r0/r1") struct spr_queue_entry * s, __reg("r2/r3") struct spr_queue_entry * target_queue)
#else
    void spr_queue_add(struct spr_queue_entry * s, struct spr_queue_entry * target_queue)
#endif
{
    #ifdef __VBCC__
        __asm(
        "\ta16\n"
        "\tx16\n"

        "\tphy\n"
        "\tlda [r0]\n"
        "\tsta [r2]\n"
        "\tldy #2\n"
        "\tlda [r0],y\n"
        "\tsta [r2],y\n"
        "\tldy #4\n"
        "\tlda [r0],y\n"
        "\tsta [r2],y\n"
        "\tldy #6\n"
        "\tlda [r0],y\n"
        "\tsta [r2],y\n"

        "\tply\n");
    #else
        target_queue->x = s->x;
        target_queue->y = s->y;
        target_queue->tileattrib = s->tileattrib;
        target_queue->depth = s->depth;
        target_queue->signsize = s->signsize;
    #endif

    return;
}

/*
    Initialize the sprite VRAM slot array
*/

void spr_init_vram_slot()
{
    for (int i = 0; i < 128; i++)
    {
        // Occupied slots are $ff
        // Slots are in the order of i, i+2, i+32, i+34, etc...
        if (i < 48)
        {
            // All entries lower than 48 are considered permanently occupied
            // (fixed for player and fixed sprites)
            // use the player's slot index which is always 0
            spr_vram_slots[i] = 0x0000;
        }
        else
        {
            // An empty slot is 0xffff.
            // A used slot will contain the object array slot
            // 32x32 sprites will take 4 such slots consecutively
            spr_vram_slots[i] = 0xffff;
        }
    }

    return;
}

// for 16x16px sprite slots
uint16_t spr_get_vram_slot_16(uint16_t i)
{
    for (int j = 48; j < 128; j++)
    {
        if (spr_vram_slots[j] == 0xffff)
        {
            spr_vram_slots[j] = i;

            //if relevant bits are taken to as they shift all the way to bit 0 first for the lowest bit:
            uint16_t temp_tilenum = 0; 
            temp_tilenum |= (j & 0xf0) << 2;
            temp_tilenum |= (j & 0x0c);
            temp_tilenum |= (j & 0x02) << 4;
            temp_tilenum |= (j & 0x01) << 1;
            // (RRRR * 64) + (CC * 4) + (r * 32) + (c * 2);
            
            objects[i].tilenum = temp_tilenum;

            objects[i].vram_addr = objects[i].tilenum << 4; // mul by 16, size of a 8x8 tile - this is in words

            return j;
        }
    }

    return 128;
}

// for 32x32px sprite slots
uint16_t spr_get_vram_slot_32(uint16_t i)
{
    for (int j = 48; j < 128; j += 4)
    {
        if ((spr_vram_slots[j] == 0xffff) && 
            (spr_vram_slots[j+1] == 0xffff) && 
            (spr_vram_slots[j+2] == 0xffff) && 
            (spr_vram_slots[j+3] == 0xffff))
        {
            spr_vram_slots[j] = i;
            spr_vram_slots[j+1] = i;
            spr_vram_slots[j+2] = i;
            spr_vram_slots[j+3] = i;

            //if relevant bits are taken to as they shift all the way to bit 0 first for the lowest bit:
            uint16_t temp_tilenum = 0; 
            temp_tilenum |= (j & 0xf0) << 2;
            temp_tilenum |= (j & 0x0c);
            temp_tilenum |= (j & 0x02) << 4;
            temp_tilenum |= (j & 0x01) << 1;
            // (RRRR * 64) + (CC * 4) + (r * 32) + (c * 2);
            
            objects[i].tilenum = temp_tilenum;

            objects[i].vram_addr = objects[i].tilenum << 4; // mul by 16, size of a 8x8 tile - this is in words

            return j;
        }
    }

    return 128;
}

void spr_release_vram_slot(uint16_t i, uint16_t slot_count)
{
    if (slot_count < 1)
    {
        slot_count = 1;
    }

    for (int j = 0; j < 128; j += slot_count)
    {
        if (spr_vram_slots[j] == i)
        {
            for (int k = j; k < j + slot_count; k++)
            {
                spr_vram_slots[k] = 0xffff;
            }
            return;
        }
    } 

    return;
}

void spr_queue_process()
{
    #ifdef __VBCC__
        __asm(
        "\ta16\n"
        "\tx16\n"
        "\tlda _spr_front_count\n"
        "\tbeq .end_drawfront\n"
        "\tphy\n"
        "\ttay\n"
        "\tlda #<_spr_queue_front\n"
        "\tsta r0\n"
        "\tlda #^_spr_queue_front\n"
        "\tsta r1\n"
        
        ".loop_drawfrontsprites:\n"
        "\tjsl >_spr_draw\n"
        "\tlda r0\n"
        "\tclc\n"
        "\tadc #8\n"
        "\tsta r0\n"
        "\tdey\n"
        "\tbne .loop_drawfrontsprites\n"
        "\tply\n"
        ".end_drawfront:\n");
    #else
        for (int i = 0; i < spr_front_count; i++)
        {
            spr_draw(&spr_queue_front[i]); 
        }
    #endif

    spr_front_count = 0;

    // Clear the depth buffer
    #ifdef __VBCC__
        __asm(
        "\ta16\n"
        "\tx8\n"
        "\tsep #$10\n"
        "\tlda #$0000\n"
        "\ttax\n"
        ".loop_depthclear:\n"
        "\tsta >_spr_depth_count,x\n"
        "\tinx\n"
        "\tinx\n"
        "\tbne .loop_depthclear\n"
        "\tsta >_spr_depth_count+255\n"
        "\trep #$10\n"
        "\tx16\n");
    #else
        for (int i = 0; i < 257; i++)
        {
            spr_depth_count[i] = 0;
        }
    #endif
    
    // Tally up sprites on each Y
    #ifdef __VBCC__
        __asm(
        "\ta16\n"
        "\tx16\n"
        "\tlda #<_spr_queue_normal\n"
        "\tsta r0\n"
        "\tlda #^_spr_queue_normal\n"
        "\tsta r1\n"
        "\tphy\n"
        "\tlda _spr_normal_count\n"
        "\tbeq .end\n"
        "\tsta r2\n"
        "\tlda #$0000\n"
        "\tldy #7\n"
        "\ta8\n"
        "\tsep #$20\n"
        "\tclc \n"
        ".loop_depthtally:\n"
        "\tlda [r0],y\n"
        "\ttax\n"
        "\tinx\n"
        "\tlda >_spr_depth_count,x\n"
        "\tinc\n"
        "\tsta >_spr_depth_count,x\n"
        "\tlda r0\n"
        "\tadc #8\n"
        "\tsta r0 \n"
        "\tbcc .depthtally_nocarry\n"
        "\tclc \n"
        "\tinc r0+1\n"
        ".depthtally_nocarry:\n"
        "\tdec r2 \n"
        "\tbne .loop_depthtally\n"
        "\ta16\n"
        "\trep #$20\n"
        ".end:\n"
        "\tply\n");
    #else
        for (int i = 0; i < spr_normal_count; i++)
        {
            spr_depth_count[spr_queue_normal[i].depth + 1]++;
        }
    #endif

    // then calculate the OAM offset for sprites
    // Correct, but must be assembly optimized
    #ifdef __VBCC__
        __asm(
        "\ta16\n"
        "\tx16\n"
        "\tphy\n"
        "\tx8\n"
        "\tsep #$10\n"
        "\tlda #$0000\n"
        "\ttax\n"
        "\tlda >_spr_sprite_count\n"
        "\tclc\n"
        "\tadc >_spr_normal_count\n"
        "\tsep #$21\n"
        "\ta8\n"
        ".loop_oamoffsetcalc:\n"
        "\tsta >_spr_depth_count,x\n"
	    "\tsbc >_spr_depth_count+1,x\n"
        "\tinx\n"
        "\tbne .loop_oamoffsetcalc\n"
        "\tsta >_spr_depth_count+256\n"
        "\trep #$30\n"
        "\ta16\n"
        "\tx16\n"
        "\tply\n");
    #else
        uint16_t temp_offset = spr_sprite_count + spr_normal_count;
        uint16_t t;

        for (int i = 0; i < 257; i++)
        {
            t = temp_offset;

            spr_depth_count[i] = t;

            temp_offset -= spr_depth_count[i + 1];
        }
    #endif

    // write out the sprites
    #ifdef __VBCC__
        __asm(
        "\ta16\n"
        "\tx16\n"
        "\tphy\n"
        "\tlda #<_spr_queue_normal\n"
        "\tsta r0\n"
        "\tlda #^_spr_queue_normal\n"
        "\tsta r1\n"
        "\tlda _spr_normal_count\n"
        "\tbeq .end2\n"
        "\tsta r2\n"
        ".loop_spritewrite:\n"
        // Decrement the depth count
        "\tlda #$0000\n"
        "\tldy #7 \n" 
        "\tsep #$20\n"
        "\ta8\n"
        "\tlda [r0],y\n"
        "\ttax\n"
        "\tlda >_spr_depth_count,x\n"
        "\tdec\n"
        "\tsta >_spr_depth_count,x\n"

        // Prepare the indices
        "\ttax\n" 

        "\trep #$20\n"
        "\ta16\n"
        "\tasl\n"
        "\tasl\n"
        "\tsta r3 \n" 
        "\tsep #$20\n"
        "\ta8\n"

        // Transfer the sprite information
        "\tldy #6\n"
        "\tlda [r0],y\n"
        "\tsta >_shadow_oam+512,x\n"
        
        "\tldx r3\n"

        "\tldy #2\n"
        "\tlda [r0],y\n"
        "\tsta >_shadow_oam+1,x\n"

        "\tlda [r0]\n"
        "\tsta >_shadow_oam,x\n"
        
        "\trep #$20\n"
        "\ta16\n"
        "\tldy #4\n" 
        "\tlda [r0],y\n"
        "\tsta >_shadow_oam+2,x\n"
        
        "\tlda r0\n"
        "\tclc \n"
        "\tadc #8\n"
        "\tsta r0 \n"
        "\tdec r2 \n"
        "\tbne .loop_spritewrite\n"
        ".end2:\n"
        "\tply\n");    
    #else
        for (int i = 0; i < spr_normal_count; i++)
        {
            spr_depth_count[spr_queue_normal[i].depth]--;

            shadow_oam.entries.shadow_oam_low[spr_depth_count[spr_queue_normal[i].depth]].tileattrib = spr_queue_normal[i].tileattrib;
            shadow_oam.entries.shadow_oam_low[spr_depth_count[spr_queue_normal[i].depth]].x = (uint8_t)spr_queue_normal[i].x;
            shadow_oam.entries.shadow_oam_low[spr_depth_count[spr_queue_normal[i].depth]].y = (uint8_t)spr_queue_normal[i].y;

            shadow_oam.entries.shadow_oam_high[spr_depth_count[spr_queue_normal[i].depth]].signsize = spr_queue_normal[i].signsize;
        }
    #endif

    spr_sprite_count += spr_normal_count;
    spr_normal_count = 0;

    #ifdef __VBCC__
        __asm(
        "\ta16\n"
        "\tx16\n"
        "\tlda _spr_back_count\n"
        "\tbeq .end_drawback\n"
        "\tphy\n"
        "\ttay\n"
        "\tlda #<_spr_queue_back\n"
        "\tsta r0\n"
        "\tlda #^_spr_queue_back\n"
        "\tsta r1\n"
        
        ".loop_drawbacksprites:\n"
        "\tjsl >_spr_draw\n"
        "\tlda r0\n"
        "\tclc\n"
        "\tadc #8\n"
        "\tsta r0\n"
        "\tdey\n"
        "\tbne .loop_drawbacksprites\n"
        "\tply\n"
        ".end_drawback:\n");
    #else
        for (int i = 0; i < spr_back_count; i++)
        {
            spr_draw(&spr_queue_back[i]); 
        }
    #endif

    spr_back_count = 0;

    return;
}

/*
    Draws a sprite immediately. Normally invoked internally from another function.
*/

#ifdef __VBCC__
    NO_INLINE void spr_draw(__reg("r0/r1") struct spr_queue_entry * s)
#else
    void spr_draw(struct spr_queue_entry * s)
#endif
{
    // Draws a sprite
    // A sprite that is here passed all checks

    #ifdef __VBCC__
        __asm(
        "\ta16\n"
	    "\tx16\n"
        "\tphy\n"

        "\tlda _spr_sprite_count\n"
        "\tasl\n"
        "\tasl\n"
        "\ttax\n"
        "\tldy #4\n"
        "\tlda [r0],y \n"
        "\tsta >_shadow_oam+2,x \n"
        "\tsep #$20\n"
        "\ta8\n"
        "\tlda [r0]\n"
        "\tsta >_shadow_oam,x \n"
        "\tldy #2\n"
        "\tlda [r0],y \n"
        "\tsta >_shadow_oam+1,x\n"
        "\tldx _spr_sprite_count\n"
        "\tldy #6\n"
        "\tlda [r0],y \n"
        "\tsta >_shadow_oam+512,x \n"
        "\tinx\n"
        "\tstx _spr_sprite_count\n"
        "\ta16\n"
        "\trep #$20\n"
        "\tply\n");
    #else
        shadow_oam.entries.shadow_oam_low[spr_sprite_count].tileattrib = s->tileattrib;
        shadow_oam.entries.shadow_oam_low[spr_sprite_count].x = (uint8_t)s->x;
        shadow_oam.entries.shadow_oam_low[spr_sprite_count].y = (uint8_t)s->y;

        shadow_oam.entries.shadow_oam_high[spr_sprite_count].signsize = s->signsize;

        spr_sprite_count++;
    #endif

    return;
}

void spr_pack_oam()
{
    // Packs high OAM bytes into the 32 bytes following the low OAM
    #ifdef __VBCC__
        __asm(
        "\tlda #<_shadow_oam+512\n"
        "\tsta r4\n"
        "\tlda #^(_shadow_oam+512)\n"
        "\tsta r4+2\n"
        "\tphy\n"
        "\tsep #$31\n"
        "\ta8\n"
        "\tx8\n"
        "\tldx #0\n"
        "\ttxy\n"
        ".loop_packoam:\n"
        "\tlda >_shadow_oam+512,x\n"
        "\tlsr\n"
        "\tlsr\n"
        "\tora >_shadow_oam+513,x\n"
        "\tlsr\n"
        "\tlsr\n"\
        "\tora >_shadow_oam+514,x\n"
        "\tlsr\n"
        "\tlsr\n"
        "\tora >_shadow_oam+515,x\n"
        "\tsta [r4],y\n"
        "\tinx\n"
        "\tinx\n"
        "\tinx\n"
        "\tinx\n"
        "\tiny\n"
        "\tcpy #32\n"
        "\tbcc .loop_packoam\n"
        "\ta16\n"
        "\tx16\n"
        "\trep #$30\n"
        "\tply\n"
        "\tstz _spr_sprite_count\n"
        );
    #else
        int j = 0;

        for (int i = 0; i < 32; i++)
        {
            shadow_oam.bytes[512+i] = 
                shadow_oam.entries.shadow_oam_high[j].signsize >> 6 |
                shadow_oam.entries.shadow_oam_high[j+1].signsize >> 4 |
                shadow_oam.entries.shadow_oam_high[j+2].signsize >> 2 |
                shadow_oam.entries.shadow_oam_high[j+3].signsize;
            j += 4;
        }

        spr_sprite_count = 0;
    #endif
    
    return;
}

/* 
    Clears all unused sprites from OAM
*/
void spr_reset_sprites()
{
    #ifdef __VBCC__
        __asm(
        "\tlda _spr_sprite_count_prev\n"
        "\tbit #3\n"
        "\tbeq .sprcount_is_already_multiple_of_four\n"
        ".loop_roundcount:\n"
        "\tinc\n"
        "\tbit #3\n"
        "\tbne .loop_roundcount\n"
        ".sprcount_is_already_multiple_of_four:\n"
        "\tsta r10\n"

        "\tlda _spr_sprite_count\n"
        "\tcmp r10\n"
        "\tbcs .end_sprreset\n"
        "\tphy\n"
        "\ttay\n"

        "\tasl\n"
        "\tasl\n"
        "\ttax\n"

        "\tlda #^(_shadow_oam+512)\n"
        "\tsta r2+2\n"
        "\tlda #<_shadow_oam+512\n"
        "\tsta r2\n"

        "\tsep #$20\n"
        "\ta8\n"
        ".loop_sprreset:\n"
        "\tlda #0\n"
        "\tsta [r2],y\n"
        "\tsta >_shadow_oam,x\n"
        "\tlda #240\n"
        "\tsta >_shadow_oam+1,x\n"

        "\tinx\n"
        "\tinx\n"
        "\tinx\n"
        "\tinx\n"
        
        "\tiny\n"
        "\tcpy _spr_sprite_count_prev\n"

        "\tbcc .loop_sprreset\n"

        "\ta16\n"
        "\trep #$20\n"

        "\tply\n"
        ".end_sprreset:\n"
        "\tlda _spr_sprite_count\n"
        "\tsta _spr_sprite_count_prev\n");
    #else
        uint16_t temp_len = spr_sprite_count_prev;
        while ((temp_len & 0x03) != 0x00)
        {
            // Round it up to the nearest multiple of 4
            temp_len++;
        }

        for (int i = spr_sprite_count; i < temp_len; i++)
        {
            // from the first unused entry to the end of previous frame's active entry.
            shadow_oam.entries.shadow_oam_low[i].y = 240;
            shadow_oam.entries.shadow_oam_high[i].signsize = 0;
        }

        spr_sprite_count_prev = spr_sprite_count;
    #endif

    // Do not reset sprite count of current frame yet, the high oam packing needs it
    return;
}
