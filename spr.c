#include <stdint.h>

#include "vars.h"

#include "spr.h"
#include "system.h"

/*
    Adds a sprite to the draw queue

    It is the responsibility of the caller select the right queue and 
    to ensure that the written sprite is valid
*/
inline void spr_queue_add_ui(int16_t x, int16_t y, uint16_t tileattrib)
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

inline void spr_queue_add_front(struct game_object * o, uint16_t tileattrib)
{
    if (spr_front_count >= 128)
    {
        return;
    }

    int16_t temp_x;
    int16_t temp_y;

    temp_x = o->pos.x.lh.h - bg_scroll_x.full.high.a;

    if ((temp_x > -16) && (temp_x < 256))
    {
        if (temp_x < 0)
        {
            spr_queue_front[spr_front_count].signsize = 0x40;
        }
        else
        {
            spr_queue_front[spr_front_count].signsize = 0x00;
        }
        temp_y = o->pos.y.lh.h - o->pos.z.lh.h - bg_scroll_y.full.high.a;

        if ((temp_y > -16) && (temp_y < 224))
        {
            spr_queue_front[spr_front_count].tileattrib = tileattrib;
            spr_queue_front[spr_front_count].x = temp_x;
            spr_queue_front[spr_front_count].y = temp_y;
            spr_queue_front[spr_front_count].depth = (temp_y + 15);

            spr_front_count++;

            return;
        }
    }

    return;
}

inline void spr_queue_add_normal(struct game_object * o, uint16_t tileattrib)
{
    if (spr_normal_count >= 128)
    {
        return;
    }

    int16_t temp_x;
    int16_t temp_y;

    temp_x = o->pos.x.lh.h - bg_scroll_x.full.high.a;

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
        temp_y = o->pos.y.lh.h - o->pos.z.lh.h - bg_scroll_y.full.high.a;

        if ((temp_y > -16) && (temp_y < 224))
        {
            spr_queue_normal[spr_normal_count].x = temp_x;
            spr_queue_normal[spr_normal_count].y = temp_y;
            spr_queue_normal[spr_normal_count].tileattrib = tileattrib;
            spr_queue_normal[spr_normal_count].depth = (temp_y + 15);

            spr_normal_count++;

            return;
        }
    }

    return;
}

inline void spr_queue_add_back(struct game_object * o, uint16_t tileattrib)
{
    if (spr_back_count >= 128)
    {
        return;
    }

    int16_t temp_x;
    int16_t temp_y;

    temp_x = o->pos.x.lh.h - bg_scroll_x.full.high.a;

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
        temp_y = o->pos.y.lh.h - o->pos.z.lh.h - bg_scroll_y.full.high.a;

        if ((temp_y > -16) && (temp_y < 224))
        {
            spr_queue_back[spr_back_count].tileattrib = tileattrib;
            spr_queue_back[spr_back_count].x = temp_x;
            spr_queue_back[spr_back_count].y = temp_y;
            spr_queue_back[spr_back_count].depth = (temp_y + 15);

            spr_back_count++;

            return;
        }
    }

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
    #if VBCC_ASM == 1
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
        "\tadc #16\n"
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
    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
        "\tx8\n"
        "\tsep #$10\n"
        
        "\tphd\n"
        "\tlda #<_spr_depth_count\n"
        "\tand #$ff00\n"
        "\tpha\n"
        "\tpld\n"
        "\ttax\n"
        "\tclc\n"
        ".loop_depthclear:\n"
        "\tstz <_spr_depth_count,x\n"
        "\tstz <_spr_depth_count+2,x\n"
        "\tstz <_spr_depth_count+4,x\n"
        "\tstz <_spr_depth_count+6,x\n"
        "\tstz <_spr_depth_count+8,x\n"
        "\tstz <_spr_depth_count+10,x\n"
        "\tstz <_spr_depth_count+12,x\n"
        "\tstz <_spr_depth_count+14,x\n"
        "\ttxa\n"
        "\tadc #16\n"
        "\ttax\n"
        "\tbne .loop_depthclear\n"
        "\tstz !_spr_depth_count+255\n"
        "\tpld\n"
        "\trep #$10\n"
        "\tx16\n");
    #else
        for (int i = 0; i < 257; i++)
        {
            spr_depth_count[i] = 0;
        }
    #endif
    
    // Tally up sprites on each Y
    #if VBCC_ASM == 1
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
        "\tldy #8\n"
        "\ta8\n"
        "\tsep #$20\n"
        "\tphb\n"
        "\tlda #^_spr_depth_count\n"
        "\tpha\n"
        "\tplb\n"
        "\tclc \n"
        ".loop_depthtally:\n"
        "\tlda [r0],y\n"
        "\ttax\n"
        "\tinx\n"
        "\tinc !_spr_depth_count,x\n"
        "\tlda r0\n"
        "\tadc #16\n"
        "\tsta r0 \n"
        "\tbcc .depthtally_nocarry\n"
        "\tclc \n"
        "\tinc r0+1\n"
        ".depthtally_nocarry:\n"
        "\tdec r2 \n"
        "\tbne .loop_depthtally\n"
        "\ta16\n"
        "\trep #$20\n"
        "\tplb\n"
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
    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
        "\tx16\n"
        "\tphd\n"
        "\tlda #<_spr_depth_count\n"
        "\tand #$ff00\n"
        "\tpha\n"
        "\tpld\n"
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
        "\ttay\n"
        ".loop_oamoffsetcalc:\n"
        "\ttya\n"
        "\tsec\n"
        "\tsta <_spr_depth_count,x\n"
	    "\tsbc <_spr_depth_count+1,x\n"
        "\tsta <_spr_depth_count+1,x\n"
	    "\tsbc <_spr_depth_count+2,x\n"
        "\tsta <_spr_depth_count+2,x\n"
	    "\tsbc <_spr_depth_count+3,x\n"
        "\tsta <_spr_depth_count+3,x\n"
	    "\tsbc <_spr_depth_count+4,x\n"
        "\tsta <_spr_depth_count+4,x\n"
	    "\tsbc <_spr_depth_count+5,x\n"
        "\tsta <_spr_depth_count+5,x\n"
	    "\tsbc <_spr_depth_count+6,x\n"
        "\tsta <_spr_depth_count+6,x\n"
	    "\tsbc <_spr_depth_count+7,x\n"
        "\tsta <_spr_depth_count+7,x\n"
	    "\tsbc <_spr_depth_count+8,x\n"
        "\tsta <_spr_depth_count+8,x\n"
	    "\tsbc <_spr_depth_count+9,x\n"
        "\tsta <_spr_depth_count+9,x\n"
	    "\tsbc <_spr_depth_count+10,x\n"
        "\tsta <_spr_depth_count+10,x\n"
	    "\tsbc <_spr_depth_count+11,x\n"
        "\tsta <_spr_depth_count+11,x\n"
	    "\tsbc <_spr_depth_count+12,x\n"
        "\tsta <_spr_depth_count+12,x\n"
	    "\tsbc <_spr_depth_count+13,x\n"
        "\tsta <_spr_depth_count+13,x\n"
	    "\tsbc <_spr_depth_count+14,x\n"
        "\tsta <_spr_depth_count+14,x\n"
	    "\tsbc <_spr_depth_count+15,x\n"
        "\tsta <_spr_depth_count+15,x\n"
	    "\tsbc <_spr_depth_count+16,x\n"
        "\ttay\n"
        "\ttxa\n"
        "\tclc\n"
        "\tadc #16\n"
        "\ttax\n"
        "\tbne .loop_oamoffsetcalc\n"
        "\ttya\n"
        "\tsta !_spr_depth_count+256\n"
        "\trep #$30\n"
        "\ta16\n"
        "\tx16\n"
        "\tply\n"
        "\tpld\n");
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
    #if VBCC_ASM == 1
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
        "\tldy #8 \n" 
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
        "\tadc #16\n"
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

    #if VBCC_ASM == 1
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
        "\tadc #16\n"
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

#if VBCC_ASM == 1
    NO_INLINE void spr_draw(__reg("r0/r1") struct spr_queue_entry * s)
#else
    void spr_draw(struct spr_queue_entry * s)
#endif
{
    // Draws a sprite
    // A sprite that is here passed all checks

    #if VBCC_ASM == 1
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
    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
	    "\tx16\n"
        "\tphd\n"
        "\tlda #<_shadow_oam+512\n"
        "\tand #$ff00\n"
        "\tpha\n"
        "\tpld\n"
        "\tphy\n"
        "\tsep #$31\n"
        "\ta8\n"
        "\tx8\n"
        "\tldx #0\n"
        "\ttxy\n"
        "\tclc\n"
        ".loop_packoam:\n"
        "\tlda <_shadow_oam+512,x\n"
        "\tlsr\n"
        "\tlsr\n"
        "\tora <_shadow_oam+513,x\n"
        "\tlsr\n"
        "\tlsr\n"\
        "\tora <_shadow_oam+514,x\n"
        "\tlsr\n"
        "\tlsr\n"
        "\tora <_shadow_oam+515,x\n"
        "\tsta _shadow_oam+512,y\n"

        "\tlda <_shadow_oam+516,x\n"
        "\tlsr\n"
        "\tlsr\n"
        "\tora <_shadow_oam+517,x\n"
        "\tlsr\n"
        "\tlsr\n"\
        "\tora <_shadow_oam+518,x\n"
        "\tlsr\n"
        "\tlsr\n"
        "\tora <_shadow_oam+519,x\n"
        "\tsta _shadow_oam+513,y\n"

        "\tlda <_shadow_oam+520,x\n"
        "\tlsr\n"
        "\tlsr\n"
        "\tora <_shadow_oam+521,x\n"
        "\tlsr\n"
        "\tlsr\n"\
        "\tora <_shadow_oam+522,x\n"
        "\tlsr\n"
        "\tlsr\n"
        "\tora <_shadow_oam+523,x\n"
        "\tsta _shadow_oam+514,y\n"

        "\tlda <_shadow_oam+524,x\n"
        "\tlsr\n"
        "\tlsr\n"
        "\tora <_shadow_oam+525,x\n"
        "\tlsr\n"
        "\tlsr\n"\
        "\tora <_shadow_oam+526,x\n"
        "\tlsr\n"
        "\tlsr\n"
        "\tora <_shadow_oam+527,x\n"
        "\tsta _shadow_oam+515,y\n"

        "\ttxa\n"
        "\tadc #16\n"
        "\ttax\n"
        "\tlsr\n"
        "\tlsr\n"
        "\ttay\n"
        "\tcpy #32\n"
        "\tbcc .loop_packoam\n"
        "\ta16\n"
        "\tx16\n"
        "\trep #$30\n"
        "\tply\n"
        "\tpld\n"
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
    #if VBCC_ASM == 1
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
