#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "ani.h"
#include "spr.h"
#include "system.h"

#include "dma.h"

NEAR struct tile_4bpp buf_player_sprite_tiles[4];

uint16_t buf_player_prev_frame;

/*
    Animations item drop gravity, and draw a drop shadow if mid-air
*/
uint16_t AniSystem_AnimateDropGravity(struct game_object * o)
{
    uint16_t grounded = 0;

    if (!((o->pos.z.a == 0) && (o->delta.z.a == 0)))
    {
        
        o->pos.z.a += o->delta.z.a;
        o->delta.z.a -= (V_GRAVITY >> 1);

        if (o->pos.z.a <= 0)
        {
            o->pos.z.a = 0;
            o->delta.z.a = 0;

            grounded = 1;
        }
    }

    if (o->pos.z.a != 0)
    {
        // also draw a shadow if relevant
        if ((o->uid & 0x0001) == ((uint16_t)system_frames_elapsed & 0x0001))
        {
            struct game_object temp;
            temp.pos.x.a = o->pos.x.a;
            temp.pos.y.a = o->pos.y.a;
            temp.pos.z.a = 0;

            uint16_t temp_tileattrib;
            temp_tileattrib = 0x0e | PAL_FX_SHADOW << 9 | 2 << 12;

            SpriteEngine_AddToBackLayer(&temp, temp_tileattrib);
        }
    }

    return grounded;
}

uint8_t * AniSystem_GetPlayerFrame(struct game_object * o)
{
    // Return player sprite address based on given information
    // State, facing
    // sprites are in the order of down, up, right, left
    // use a virtual tilenum system before finalizing.
    uint16_t temp_tilenum = 0;

    switch (o->state)
    {
        case STATE_IDLE:
            break;
        case STATE_MOVE_WALK:
            temp_tilenum += 4;
            break;
        case STATE_MOVE_RUN:
            temp_tilenum += 40;
            break;
        case STATE_ATTACK_BASIC:
            temp_tilenum += 24;
            break;
        case STATE_ATTACK_BASIC_MOVE:
            temp_tilenum += 32;
            break;
        case STATE_ATTACK_SPECIAL:
            temp_tilenum += 12;
            break;
        case STATE_ATTACK_SPECIAL_MOVE:
            temp_tilenum += 16;
            break;
        case STATE_HURT_NORMAL:
            temp_tilenum += 48;
            break;
        case STATE_HURT_NORMAL_MOVE:
            temp_tilenum += 52;
            break;
        case STATE_HURT_NORMAL_MOVE_RUN:
            temp_tilenum += 60;
            break;
        case STATE_HURT_BURN:
            temp_tilenum += 48;
            break;
        case STATE_HURT_BURN_MOVE:
            temp_tilenum += 52;
            break;
        case STATE_ICON_NORMAL:
            temp_tilenum += 68;
            break;
        case STATE_ICON_BLINK:
            temp_tilenum += 69;
            break;
        case STATE_ICON_HURT:
            temp_tilenum += 70;
            break;
        case STATE_ICON_SPECIAL:
            temp_tilenum += 71;
            break;
        case STATE_DIE:
            temp_tilenum += 72;
            break;
    }

    if (o->state != STATE_DIE)
    {
        if (o->state < STATE_ICON_NORMAL)
        {
            if (o->state == STATE_IDLE || o->state == STATE_ATTACK_SPECIAL || o->state == STATE_HURT_NORMAL)
            {
                switch (o->facing)
                {
                    case FACING_DOWN:
                        break;
                    case FACING_UP:
                        temp_tilenum += 1;
                        break;
                    case FACING_RIGHT:
                        temp_tilenum += 2;
                        break;
                    case FACING_LEFT:
                        temp_tilenum += 3;
                        break;
                }
            }
            else
            {
                switch (o->facing)
                {
                    case FACING_DOWN:
                        break;
                    case FACING_UP:
                        temp_tilenum += 2;
                        break;
                    case FACING_RIGHT:
                        temp_tilenum += 4;
                        break;
                    case FACING_LEFT:
                        temp_tilenum += 6;
                        break;
                }
            }

            // Now add the frame offset.
            temp_tilenum += o->struct_data.npc_data.ani.frame;
        }
    }
    else
    {
        // Now add the frame offset.
        // Still needed
        temp_tilenum += o->struct_data.npc_data.ani.frame;
    }

    // Fetch the compressed frame
    return AniSystem_GetCompressedFrame((const uint8_t *)&data_spr_player_dd, (const uint16_t *)&data_spr_player_lut, temp_tilenum);
}

uint8_t * AniSystem_GetDynamicFrame(struct game_object * o)
{
    switch (o->id)
    {
        case OBJID_SLIME:
            return AniSystem_GetDynamicFrame_Slime(o);
        case OBJID_LIZARDMAN:
            return AniSystem_GetDynamicFrame_Lizardman(o);
        default:
            return 0;
    }
}

uint8_t * AniSystem_GetDynamicFrame_Stateless(struct game_object * o)
{
    switch (o->id)
    {
        case OBJID_BUBBLE_E:
            return AniSystem_GetDynamicFrame_Bubble(o);
        case OBJID_ARROW_E:
            return AniSystem_GetDynamicFrame_Arrow(o);
        default:
            return 0;
    }
}

// Return offset to a fixed sprite tilenum based on given information
// object ID and frame only
// shorter version for light objects
FORCE_INLINE uint16_t AniSystem_GetFixedFrame_Fast(struct game_object * o)
{
    switch (o->id)
    {
        case OBJID_FX_SMOKE:
            return 6+(o->struct_data.npc_data.ani.frame << 1);
        case OBJID_FIREBALL:
            return 2+(o->struct_data.npc_data.ani.frame << 1);
        case OBJID_SYS_IMPACT:
            return 10;
        case OBJID_SYS_TARGET:
            return 14; 
        default:
            return 0;
    }
}

#if VBCC_ASM == 1
NO_INLINE uint8_t * AniSystem_GetDynamicFrame_Bubble(struct game_object * o)
#else
uint8_t * AniSystem_GetDynamicFrame_Bubble(struct game_object * o)
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\ttax\n"
            // 30 + Frame Number
            // Current frame is byte 64
            "\tlda #30\n"
            "\tclc\n"
            "\tadc $7e0040,x\n" // current frame. now we have the tile num in virtual space

            "\ttxy\n"
            "\tasl\n" // This clears the carry
            "\ttax\n"
            "\tlda >_const_ani_lut_frame_byteoffsets_16,x\n"
            "\ttyx\n"

            "\tadc #<_data_spr_slime\n"
            "\tldx #^_data_spr_slime\n"

            ".finish:\n"

            "\trtl\n"
        );
    #else
        // use a virtual tilenum system before finalizing.
        uint16_t temp_tilenum = 30;

        // Now add the frame offset.
        temp_tilenum += o->struct_data.npc_data.ani.frame;

        // Calculate the address
        return (uint8_t *)((uint32_t)&data_spr_slime + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10));
    #endif

    return 0;
}

uint8_t * AniSystem_GetDynamicFrame_Arrow(struct game_object * o)
{
    // use a virtual tilenum system before finalizing.
    // Arrow up is 32, arrow right is 36
    bool hflip = false;
    bool vflip = false;

    uint8_t angle = o->angle + 64; // Angle offset
    uint16_t temp_tilenum = 32;

    // get the object angle
    if (angle >= 240+8)
    {
        temp_tilenum = 32;
    }
    else if (angle >= 224+8)
    {
        temp_tilenum = 33;
        hflip = true;
    }
    else if (angle >= 208+8)
    {
        temp_tilenum = 34;
        hflip = true;
    }
    else if (angle >= 192+8)
    {
        temp_tilenum = 35;
        hflip = true;
    }
    else if (angle >= 176+8)
    {
        temp_tilenum = 36;
        hflip = true;
    }
    else if (angle >= 160+8)
    {
        temp_tilenum = 35;
        hflip = true;
        vflip = true;
    }
    else if (angle >= 144+8)
    {
        temp_tilenum = 34;
        hflip = true;
        vflip = true;
    }
    else if (angle >= 128+8)
    {
        temp_tilenum = 33;
        hflip = true;
        vflip = true;
    }
    else if (angle >= 112+8)
    {
        temp_tilenum = 32;
        vflip = true;
    }
    else if (angle >= 96+8)
    {
        temp_tilenum = 33;
        vflip = true;
    }
    else if (angle >= 80+8)
    {
        temp_tilenum = 34;
        vflip = true;
    }
    else if (angle >= 64+8)
    {
        temp_tilenum = 35;
        vflip = true;
    }
    else if (angle >= 48+8)
    {
        temp_tilenum = 36;
    }
    else if (angle >= 32+8)
    {
        temp_tilenum = 35;
    }
    else if (angle >= 16+8)
    {
        temp_tilenum = 34;
    }
    else if (angle >= 0+8)
    {
        temp_tilenum = 33;
    }
    else
    {
        temp_tilenum = 32;
    }

    uint32_t temp_addr = ((uint32_t)&data_spr_lizardman + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10));

    if (hflip)
    {
        temp_addr |= 0x40000000; // Set second highest bit
    }
    if (vflip)
    {
        temp_addr |= 0x80000000; // Set highest bit
    }

    temp_addr |= ((uint32_t)temp_tilenum - 32l) << 24; // use 6 bits of the highest bytes to store the tile number minus 32

    // Calculate the address
    return (uint8_t *)temp_addr;
}

#if VBCC_ASM == 1
NO_INLINE uint8_t * AniSystem_GetDynamicFrame_Slime(__reg("a/x") struct game_object * o)
#else
uint8_t * AniSystem_GetDynamicFrame_Slime(struct game_object * o)
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\ttax\n"

            "\tlda $7e001e,x\n"
            "\tcmp #12\n"
            "\tbne .not_spawning\n"

            ".spawning:\n"
                // Frame Number
                // Current frame is byte 64
                "\tlda $7e0040,x\n" // current frame. now we have the tile num in virtual space
                "\tasl\n" // This clears the carry
                "\ttax\n"
                "\tlda >_const_ani_lut_frame_byteoffsets_16,x\n"
                "\tadc #<_data_spr_spawn_placeholder\n"
                "\tldx #^_data_spr_spawn_placeholder\n"

                "\tbra .finish\n"

            ".not_spawning:\n"
                // (State * 4 + Facing + Frame Number)
                // state is byte 30, facing is byte 32, current frame is byte 64
                // The state value is still loaded at this point
                "\tasl\n"
                "\tasl\n" // Carry is cleared here
                "\tadc $7e0020,x\n" // facing
                
                "\tasl\n" // Now we have the index to look into the lookup
                "\ttxy\n"
                "\ttax\n"
                "\tlda >_const_ani_lut_basic, x\n"

                // Transform this number
                // (temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10)
                "\ttyx\n"
                "\tadc $7e0040,x\n" // current frame. now we have the tile num in virtual space

                "\ttxy\n"
                "\tasl\n" // This clears the carry
                "\ttax\n"
                "\tlda >_const_ani_lut_frame_byteoffsets_16,x\n"
                "\ttyx\n"
                
                "\tadc #<_data_spr_slime\n" 

                "\ttay\n"

                // Test for sign flip
                "\tlda $7e001e,x\n" // state
                "\tcmp #13\n"
                "\tbeq .no_flip\n"
                "\tlda $7e0020,x\n" // facing
                "\tcmp #3\n"
                "\tbne .no_flip\n"
                ".flip_x:\n"
                    "\tlda #$8000\n"
                    "\tora #^_data_spr_slime\n"
                    "\ttax\n"
                    "\tbra .finalize\n"
                ".no_flip:\n"
                    "\tldx #^_data_spr_slime\n"

            ".finalize:\n"
            "\ttya\n"

            ".finish:\n"

            "\trtl\n"
        );
    #else
        // use a virtual tilenum system before finalizing.
        uint16_t temp_tilenum = o->struct_data.npc_data.ani.frame; // add the frame offset.

        if (o->state == STATE_SPAWNING)
        {
            return (uint8_t *)((uint32_t)&data_spr_spawn_placeholder + const_ani_lut_frame_byteoffsets_16[temp_tilenum]);
            //return (uint8_t *)((uint32_t)&data_spr_spawn_placeholder + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10));
        }
        else
        {
            temp_tilenum += const_ani_lut_basic[(o->state << 2) + o->facing];

            if ((o->facing == FACING_LEFT) && (o->state != STATE_DIE))
            {
                return (uint8_t *)(((uint32_t)&data_spr_slime + const_ani_lut_frame_byteoffsets_16[temp_tilenum]) | 0x80000000); // set the negative flag
                //return (uint8_t *)(((uint32_t)&data_spr_slime + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10)) | 0x80000000); // set the negative flag
            }
            else
            {
                // Calculate the address
                return (uint8_t *)((uint32_t)&data_spr_slime + const_ani_lut_frame_byteoffsets_16[temp_tilenum]);
                //return (uint8_t *)((uint32_t)&data_spr_slime + ((temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10));
            }
        }
    #endif

    return 0;
}


#if VBCC_ASM == 1
NO_INLINE uint8_t * AniSystem_GetDynamicFrame_Lizardman(__reg("a/x") struct game_object * o)
#else
uint8_t * AniSystem_GetDynamicFrame_Lizardman(struct game_object * o)
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tx16\n"

            "\ttax\n"

            "\tlda $7e001e,x\n"
            "\tcmp #12\n"
            "\tbne .not_spawning\n"

            ".spawning:\n"
                // Frame Number
                // Current frame is byte 64
                "\tlda $7e0040,x\n" // current frame. now we have the tile num in virtual space
                "\tasl\n" // This clears the carry
                "\ttax\n"
                "\tlda >_const_ani_lut_frame_byteoffsets_16,x\n"
                "\tadc #<_data_spr_spawn_placeholder\n"
                "\tldx #^_data_spr_spawn_placeholder\n"

                "\tbra .finish\n"

            ".not_spawning:\n"
                // (State * 4 + Facing + Frame Number)
                // state is byte 30, facing is byte 32, current frame is byte 64
                // The state value is still loaded at this point
                "\tasl\n"
                "\tasl\n" // Carry is cleared here
                "\tadc $7e0020,x\n" // facing
                
                "\tasl\n" // Now we have the index to look into the lookup
                "\ttxy\n"
                "\ttax\n"
                "\tlda >_const_ani_lut_lizardman, x\n"

                // Transform this number
                // (temp_tilenum & 0x07) << 6) + ((temp_tilenum >> 3) << 10)
                "\ttyx\n"
                "\tadc $7e0040,x\n" // current frame. now we have the tile num in virtual space

                "\ttxy\n"
                "\tasl\n" // This clears the carry
                "\ttax\n"
                "\tlda >_const_ani_lut_frame_byteoffsets_16,x\n"
                "\ttyx\n"
                
                "\tadc #<_data_spr_lizardman\n" 

                "\ttay\n"

                // Test for sign flip
                "\tlda $7e001e,x\n" // state
                "\tcmp #13\n"
                "\tbeq .no_flip\n"
                "\tlda $7e0020,x\n" // facing
                "\tcmp #3\n"
                "\tbne .no_flip\n"
                ".flip_x:\n"
                    "\tlda #$8000\n"
                    "\tora #^_data_spr_lizardman\n"
                    "\ttax\n"
                    "\tbra .finalize\n"
                ".no_flip:\n"
                    "\tldx #^_data_spr_lizardman\n"

            ".finalize:\n"
            "\ttya\n"

            ".finish:\n"

            "\trtl\n"
        );
    #else
        // use a virtual tilenum system before finalizing.
        uint16_t temp_tilenum = o->struct_data.npc_data.ani.frame; // add the frame offset.

        if (o->state == STATE_SPAWNING)
        {
            return (uint8_t *)((uint32_t)&data_spr_spawn_placeholder + const_ani_lut_frame_byteoffsets_16[temp_tilenum]);
        }
        else
        {
            temp_tilenum += const_ani_lut_lizardman[(o->state << 2) + o->facing];

            if ((o->facing == FACING_LEFT) && (o->state != STATE_DIE))
            {
                return (uint8_t *)(((uint32_t)&data_spr_lizardman + const_ani_lut_frame_byteoffsets_16[temp_tilenum]) | 0x80000000); // set the negative flag
            }
            else
            {
                // Calculate the address
                return (uint8_t *)((uint32_t)&data_spr_lizardman + const_ani_lut_frame_byteoffsets_16[temp_tilenum]);
            }
        }
    #endif

    return 0;
}

// Fetch and build compressed frame in WRAM
// This will return the index in the lookup table for the purposes of checking prev frames
uint8_t * AniSystem_GetCompressedFrame(const uint8_t * data, const uint16_t * lookup, uint16_t frame)
{
    uint16_t lookup_entry_offset = frame << 2; // Each frame is 8 bytes

    struct tile_4bpp * ptr_read;

    uint8_t * ptr_return_val = (uint8_t *)(lookup + lookup_entry_offset);

    if (frame == buf_player_prev_frame)
    {
        return ptr_return_val;
    }

    buf_player_prev_frame = frame;

    uint16_t * data_offset = (uint16_t *)ptr_return_val;

    struct tile_4bpp * ptr_read_0 = (struct tile_4bpp *)(data + *data_offset++);
    struct tile_4bpp * ptr_read_1 = (struct tile_4bpp *)(data + *data_offset++);
    struct tile_4bpp * ptr_read_2 = (struct tile_4bpp *)(data + *data_offset++);
    struct tile_4bpp * ptr_read_3 = (struct tile_4bpp *)(data + *data_offset);

    // Use the DMA unit to speed things up. Channel 7 is reserved for active display DMA.
    // Align read
    REG_DMAP7 = 0x00; // byte reg write
    REG_BBAD7 = 0x80; // WMDATA

    REG_WMADDLM = (uint16_t)&buf_player_sprite_tiles[0];
    REG_WMADDH = 0x00;

    REG_A1T7LH = (uint16_t)((uint32_t)ptr_read_0);
    REG_A1B7 = (uint8_t)((uint32_t)ptr_read_0 >> 16);
    
    REG_DAS7LH = 32;

    while ((REG_HVBJOY & HBL_READY) == HBL_READY)
    {
        ;
    }

    while ((REG_HVBJOY & HBL_READY) == 0x00)
    {
        ;
    }
    REG_MDMAEN = 0x80;

    REG_A1T7LH = (uint16_t)((uint32_t)ptr_read_1);
    
    REG_DAS7LH = 32;
    REG_MDMAEN = 0x80;

    REG_A1T7LH = (uint16_t)((uint32_t)ptr_read_2);
    
    REG_DAS7LH = 32;
    REG_MDMAEN = 0x80;

    REG_A1T7LH = (uint16_t)((uint32_t)ptr_read_3);
    
    REG_DAS7LH = 32;
    REG_MDMAEN = 0x80;

    return ptr_return_val;
}

// Lookup tables for animations
/*
    For reference:

    #define STATE_IDLE 0

    #define STATE_MOVE_WALK 1 // Yes, there is overlap
    #define STATE_MOVE_RUN 2

    #define STATE_ATTACK_BASIC 3
    #define STATE_ATTACK_BASIC_MOVE 4

    #define STATE_ATTACK_SPECIAL 5
    #define STATE_ATTACK_SPECIAL_MOVE 6

    #define STATE_HURT_NORMAL 7
    #define STATE_HURT_NORMAL_MOVE 8
    #define STATE_HURT_NORMAL_MOVE_RUN 9

    #define STATE_HURT_BURN 10
    #define STATE_HURT_BURN_MOVE 11

    #define STATE_SPAWNING 12
    #define STATE_DIE 13

    #define FACING_DOWN 0
    #define FACING_UP 1
    #define FACING_RIGHT 2
    #define FACING_LEFT 3
*/

// With flipping
// Used for the slime and any other enemy with similar set up
NEAR const uint16_t const_ani_lut_basic[56] = 
{
   0, 1, 2, 2,

   3, 5, 7, 7,
   3, 5, 7, 7,

   18, 20, 22, 22,
   18, 20, 22, 22,

   24, 26, 28, 28,
   24, 26, 28, 28,
    
   0, 1, 2, 2,
   3, 5, 7, 7,
   3, 5, 7, 7,

   9, 10, 11, 11,
   12, 14, 16, 16,

   0, 0, 0, 0,
   32, 32, 32, 32,
};

// Used for the lizardman
NEAR const uint16_t const_ani_lut_lizardman[56] = 
{
    0, 1, 2, 2,
 
    3, 5, 7, 7,
    3, 5, 7, 7,
 
    9, 11, 13, 13,
    9, 11, 13, 13,
 
    37, 38, 39, 39,
    37, 38, 39, 39,
     
    0, 1, 2, 2,
    3, 5, 7, 7,
    3, 5, 7, 7,
 
    15, 16, 17, 17,
    18, 20, 22, 22,
 
    0, 0, 0, 0,
    24, 24, 24, 24,
 };

// Byte offset tables large enough to cover an entire 64KB bank
NEAR const uint16_t const_ani_lut_frame_byteoffsets_16[512] =
{
    0x0000, 0x0040, 0x0080, 0x00c0, 0x0100, 0x0140, 0x0180, 0x01c0, 
    0x0400, 0x0440, 0x0480, 0x04c0, 0x0500, 0x0540, 0x0580, 0x05c0, 
    0x0800, 0x0840, 0x0880, 0x08c0, 0x0900, 0x0940, 0x0980, 0x09c0, 
    0x0c00, 0x0c40, 0x0c80, 0x0cc0, 0x0d00, 0x0d40, 0x0d80, 0x0dc0, 

    0x1000, 0x1040, 0x1080, 0x10c0, 0x1100, 0x1140, 0x1180, 0x11c0, 
    0x1400, 0x1440, 0x1480, 0x14c0, 0x1500, 0x1540, 0x1580, 0x15c0, 
    0x1800, 0x1840, 0x1880, 0x18c0, 0x1900, 0x1940, 0x1980, 0x19c0, 
    0x1c00, 0x1c40, 0x1c80, 0x1cc0, 0x1d00, 0x1d40, 0x1d80, 0x1dc0, 
    
    0x2000, 0x2040, 0x2080, 0x20c0, 0x2100, 0x2140, 0x2180, 0x21c0, 
    0x2400, 0x2440, 0x2480, 0x24c0, 0x2500, 0x2540, 0x2580, 0x25c0, 
    0x2800, 0x2840, 0x2880, 0x28c0, 0x2900, 0x2940, 0x2980, 0x29c0, 
    0x2c00, 0x2c40, 0x2c80, 0x2cc0, 0x2d00, 0x2d40, 0x2d80, 0x2dc0, 

    0x3000, 0x3040, 0x3080, 0x30c0, 0x3100, 0x3140, 0x3180, 0x31c0, 
    0x3400, 0x3440, 0x3480, 0x34c0, 0x3500, 0x3540, 0x3580, 0x35c0, 
    0x3800, 0x3840, 0x3880, 0x38c0, 0x3900, 0x3940, 0x3980, 0x39c0, 
    0x3c00, 0x3c40, 0x3c80, 0x3cc0, 0x3d00, 0x3d40, 0x3d80, 0x3dc0, 

    0x4000, 0x4040, 0x4080, 0x40c0, 0x4100, 0x4140, 0x4180, 0x41c0, 
    0x4400, 0x4440, 0x4480, 0x44c0, 0x4500, 0x4540, 0x4580, 0x45c0, 
    0x4800, 0x4840, 0x4880, 0x48c0, 0x4900, 0x4940, 0x4980, 0x49c0, 
    0x4c00, 0x4c40, 0x4c80, 0x4cc0, 0x4d00, 0x4d40, 0x4d80, 0x4dc0, 

    0x5000, 0x5040, 0x5080, 0x50c0, 0x5100, 0x5140, 0x5180, 0x51c0, 
    0x5400, 0x5440, 0x5480, 0x54c0, 0x5500, 0x5540, 0x5580, 0x55c0, 
    0x5800, 0x5840, 0x5880, 0x58c0, 0x5900, 0x5940, 0x5980, 0x59c0, 
    0x5c00, 0x5c40, 0x5c80, 0x5cc0, 0x5d00, 0x5d40, 0x5d80, 0x5dc0, 
    
    0x6000, 0x6040, 0x6080, 0x60c0, 0x6100, 0x6140, 0x6180, 0x61c0, 
    0x6400, 0x6440, 0x6480, 0x64c0, 0x6500, 0x6540, 0x6580, 0x65c0, 
    0x6800, 0x6840, 0x6880, 0x68c0, 0x6900, 0x6940, 0x6980, 0x69c0, 
    0x6c00, 0x6c40, 0x6c80, 0x6cc0, 0x6d00, 0x6d40, 0x6d80, 0x6dc0, 

    0x7000, 0x7040, 0x7080, 0x70c0, 0x7100, 0x7140, 0x7180, 0x71c0, 
    0x7400, 0x7440, 0x7480, 0x74c0, 0x7500, 0x7540, 0x7580, 0x75c0, 
    0x7800, 0x7840, 0x7880, 0x78c0, 0x7900, 0x7940, 0x7980, 0x79c0, 
    0x7c00, 0x7c40, 0x7c80, 0x7cc0, 0x7d00, 0x7d40, 0x7d80, 0x7dc0,

    0x8000, 0x8040, 0x8080, 0x80c0, 0x8100, 0x8140, 0x8180, 0x81c0, 
    0x8400, 0x8440, 0x8480, 0x84c0, 0x8500, 0x8540, 0x8580, 0x85c0, 
    0x8800, 0x8840, 0x8880, 0x88c0, 0x8900, 0x8940, 0x8980, 0x89c0, 
    0x8c00, 0x8c40, 0x8c80, 0x8cc0, 0x8d00, 0x8d40, 0x8d80, 0x8dc0, 

    0x9000, 0x9040, 0x9080, 0x90c0, 0x9100, 0x9140, 0x9180, 0x91c0, 
    0x9400, 0x9440, 0x9480, 0x94c0, 0x9500, 0x9540, 0x9580, 0x95c0, 
    0x9800, 0x9840, 0x9880, 0x98c0, 0x9900, 0x9940, 0x9980, 0x99c0, 
    0x9c00, 0x9c40, 0x9c80, 0x9cc0, 0x9d00, 0x9d40, 0x9d80, 0x9dc0, 
    
    0xa000, 0xa040, 0xa080, 0xa0c0, 0xa100, 0xa140, 0xa180, 0xa1c0, 
    0xa400, 0xa440, 0xa480, 0xa4c0, 0xa500, 0xa540, 0xa580, 0xa5c0, 
    0xa800, 0xa840, 0xa880, 0xa8c0, 0xa900, 0xa940, 0xa980, 0xa9c0, 
    0xac00, 0xac40, 0xac80, 0xacc0, 0xad00, 0xad40, 0xad80, 0xadc0, 

    0xb000, 0xb040, 0xb080, 0xb0c0, 0xb100, 0xb140, 0xb180, 0xb1c0, 
    0xb400, 0xb440, 0xb480, 0xb4c0, 0xb500, 0xb540, 0xb580, 0xb5c0, 
    0xb800, 0xb840, 0xb880, 0xb8c0, 0xb900, 0xb940, 0xb980, 0xb9c0, 
    0xbc00, 0xbc40, 0xbc80, 0xbcc0, 0xbd00, 0xbd40, 0xbd80, 0xbdc0, 

    0xc000, 0xc040, 0xc080, 0xc0c0, 0xc100, 0xc140, 0xc180, 0xc1c0, 
    0xc400, 0xc440, 0xc480, 0xc4c0, 0xc500, 0xc540, 0xc580, 0xc5c0, 
    0xc800, 0xc840, 0xc880, 0xc8c0, 0xc900, 0xc940, 0xc980, 0xc9c0, 
    0xcc00, 0xcc40, 0xcc80, 0xccc0, 0xcd00, 0xcd40, 0xcd80, 0xcdc0, 

    0xd000, 0xd040, 0xd080, 0xd0c0, 0xd100, 0xd140, 0xd180, 0xd1c0, 
    0xd400, 0xd440, 0xd480, 0xd4c0, 0xd500, 0xd540, 0xd580, 0xd5c0, 
    0xd800, 0xd840, 0xd880, 0xd8c0, 0xd900, 0xd940, 0xd980, 0xd9c0, 
    0xdc00, 0xdc40, 0xdc80, 0xdcc0, 0xdd00, 0xdd40, 0xdd80, 0xddc0, 
    
    0xe000, 0xe040, 0xe080, 0xe0c0, 0xe100, 0xe140, 0xe180, 0xe1c0, 
    0xe400, 0xe440, 0xe480, 0xe4c0, 0xe500, 0xe540, 0xe580, 0xe5c0, 
    0xe800, 0xe840, 0xe880, 0xe8c0, 0xe900, 0xe940, 0xe980, 0xe9c0, 
    0xec00, 0xec40, 0xec80, 0xecc0, 0xed00, 0xed40, 0xed80, 0xedc0, 

    0xf000, 0xf040, 0xf080, 0xf0c0, 0xf100, 0xf140, 0xf180, 0xf1c0, 
    0xf400, 0xf440, 0xf480, 0xf4c0, 0xf500, 0xf540, 0xf580, 0xf5c0, 
    0xf800, 0xf840, 0xf880, 0xf8c0, 0xf900, 0xf940, 0xf980, 0xf9c0, 
    0xfc00, 0xfc40, 0xfc80, 0xfcc0, 0xfd00, 0xfd40, 0xfd80, 0xfdc0,
};