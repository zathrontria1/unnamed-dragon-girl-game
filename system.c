#include <stdint.h>
#include <snes/console.h>

#include "vars.h"

#include "dma.h"
#include "interrupt.h"
#include "system.h"
#include "lz4.h"
#include "asm.h"
#include "spr.h"
#include "obj.h"
#include "routines.h"
#include "loop.h"
#include "map.h"
#include "math_int.h"

#include "ani_pal.h"
#include "ani_pal_hdma.h"

#include "snd.h"
#include "ui.h"

uint8_t system_MVNCodeInWRAM[4];
uint8_t system_JMLCodeInWRAM[4];

/*
    VBCC doesn't initialize the zero page variables
    So do it for it
*/
void system_init_zp()
{
    system_in_vblank = 0;
    system_current_routine = 0;
    system_target_routine = 0;
    system_frames_elapsed = 0;
    shadow_stat77 = 0;
    shadow_inidisp = 0;
    system_nmis_counted = 0;
    bg_scroll_x.a = 0;
    bg_scroll_y.a = 0;
    bg_scroll_y_mod.a = 0;
    ani_bg_water_dma_ready = 0;
    ani_bg_tallbg_dma_ready = 0;

    obj_first_available = 0;

    return;
}

/*
    Initialize all system registers. In case something else was running beforehand

    TODO: if to be released outside of SNES this will need to be #ifdef'd
*/
void system_init_regs(void)
{
    // Must do first
    REG_NMITIMEN = 0x00; // Disable interrupts
    REG_HDMAEN = 0x00; // Disable HDMA
    REG_INIDISP = 0x8f; // Disable the screen - stdio might turn it on

    REG_MEMSEL = 0x01; // Enable fastROM
    REG_WRIO = 0xff;

    REG_OBSEL = 0x00;
    REG_OAMADDLH = 0x0000;

    REG_BGMODE = 0x00;
    REG_MOSAIC = 0x00;

    REG_BG1SC = 0x00;
    REG_BG2SC = 0x00;
    REG_BG3SC = 0x00;
    REG_BG4SC = 0x00;

    REG_BG12NBA = 0x00;
    REG_BG34NBA = 0x00;

    system_reset_bg_scroll_regs();

    REG_VMAIN = 0x80;

    REG_M7SEL = 0x00;

    REG_M7A = 0;
    REG_M7A = 1;

    REG_M7B = 0;
    REG_M7B = 0;

    REG_M7C = 0;
    REG_M7C = 0;

    REG_M7D = 0;
    REG_M7D = 1;

    REG_M7X = 0;
    REG_M7X = 0;

    REG_M7Y = 0;
    REG_M7Y = 0;

    REG_W12SEL = 0;
    REG_W34SEL = 0;
    REG_WOBJSEL = 0;
    REG_WH0 = 0;
    REG_WH1 = 0;
    REG_WH2 = 0;
    REG_WH3 = 0;
    REG_WBGLOG = 0;
    REG_WOBJLOG = 0;

    REG_TM = 0;
    REG_TS = 0;
    REG_TMW = 0;
    REG_TSW = 0;

    REG_CGWSEL = 0x30;
    REG_CGADSUB = 0;
    REG_COLDATA = 0xe0;

    REG_SETINI = 0;

    return;
}

void system_init()
{
    // Write out the MVN and JML program codes
    #if VBCC_ASM == 1
    __asm(
        "\ta16\n"
	    "\tx16\n"
        "\ta8\n"
        "\tsep #$20\n"
        "\tlda #$6B\n" // RTL opcode
        "\tsta >_system_MVNCodeInWRAM+3\n"
        "\tlda #$54\n" // MVN opcode
        "\tsta >_system_MVNCodeInWRAM\n"
        "\tlda #$5c\n" // JML opcode
        "\tsta >_system_JMLCodeInWRAM\n"
        "\ta16\n"
        "\trep #$20\n");
    #else
        system_MVNCodeInWRAM[0] = 0x54;
        system_MVNCodeInWRAM[3] = 0x6b;

        system_JMLCodeInWRAM[0] = 0x5c;
    #endif

    // Set current and target routines to init to prevent issues
    system_current_routine = ROUTINE_INIT;
    system_target_routine = ROUTINE_INIT;

    // Clear video memory
    dma_clear_vram();

    spr_init_vram_slot();

    // Copy the sprite data into VRAM
    LZ4_UnpackToVRAM((void *)&data_sprite_fixed_lz4, TILEDATA_ADDR_SPRITES); // permanent effects and system, upload first
    // Most sprites are dynamically allocated

    // Write BG1, BG2, BG3 and BG4 scroll to 0 on X and negative 1 on Y axis
    system_reset_bg_scroll_regs();

    // Reset sprites
    spr_sprite_count_prev = 128;
    spr_sprite_count = 0;
    spr_reset_sprites();
    spr_pack_oam();

    // Set up sprite display
    REG_OBSEL = OBJ_SIZE16_L32|3;

    // Reset object system
    obj_reset();
    obj_player_index = 0xffff;

    // Initialize BG scroll systems. Must be done before the map is loaded.
    bg_scroll_x_bounds_min.full.high.a = -32768;
    bg_scroll_y_bounds_min.full.high.a = -32768;
    bg_scroll_use_interpolation = 0;

    bg_scroll_x.a = 0;
    bg_scroll_y.a = 0;
    bg_scroll_x_prev.a = 0;
    bg_scroll_y_prev.a = 0;

    // invalidate UI caches
    ui_cached_hp = -1;
    ui_cached_hp_max = -1;
    ui_cached_money = 4294967295;
    ui_cached_enemy_counter = 65535;

    // initialize UI DMA tiles
    ui_dma_ui_tiles();

    return;
}

void system_reset_bg_scroll_regs(void)
{
    REG_BG1HOFS = 0;
    REG_BG1HOFS = 0;
    REG_BG2HOFS = 0;
    REG_BG2HOFS = 0;
    REG_BG3HOFS = 0;
    REG_BG3HOFS = 0;
    REG_BG4HOFS = 0;
    REG_BG4HOFS = 0;

    REG_BG1VOFS = 0xff;
    REG_BG1VOFS = 0xff;
    REG_BG2VOFS = 0xff;
    REG_BG2VOFS = 0xff;
    REG_BG3VOFS = 0xff;
    REG_BG3VOFS = 0xff;
    REG_BG4VOFS = 0xff;
    REG_BG4VOFS = 0xff;

    return;
}

/*
    Call when the display mode is to be changed
*/
void system_init_display(uint16_t routine)
{
    switch (routine)
    {
        case ROUTINE_GAMELOOP:
            REG_BGMODE = 0x09; // Mode 1, high priority bg3
            REG_TM = TM_MODE1; // BG1, BG2, BG3, and OBJ
            break;
        case ROUTINE_MAPDISPLAY:
            REG_BGMODE = 0x03; // Mode 3
            REG_TM = TM_MODE3; // BG1, BG2, and OBJ
            break;
    }

    return;
}

// Set background tile entries and tilemap locations
void system_setup_tilemap_display(uint16_t routine)
{
    switch (routine)
    {
        case ROUTINE_GAMELOOP:
            REG_BG12NBA = 0 << 4 | 4;
            REG_BG34NBA = (4 << 4 | 4);

            REG_BG1SC = TILEMAP_ADDR_GAME_UI_4BPP >> 8;
            REG_BG2SC = TILEMAP_ADDR_GAME_MAP >> 8 | 1;
            REG_BG3SC = TILEMAP_ADDR_GAME_UI_2BPP >> 8;
            break;
        case ROUTINE_MAPDISPLAY:
            REG_BG12NBA = 4 << 4 | 0;

            REG_BG1SC = TILEMAP_ADDR_MAP_MAP >> 8;
            REG_BG2SC = TILEMAP_ADDR_MAP_UI >> 8;
            break;
    }

    return;
}

void system_wait_vblank()
{
    // A workaround has been done on the ASM code side
    system_in_vblank = 1; // This must be the last value written.

    while (system_in_vblank)
    {
        emitWAI();
    }  

    system_poll_input();
        
    return;
}

inline void system_poll_input()
{
    // Check if input is ready.
    while ((REG_HVBJOY & PAD_BUSY) == PAD_BUSY)
        ;

    // Store last frame's input temporarily.
    uint16_t temp_pad0 = input_pad0; 

    // Current frame input
    input_pad0 = REG_JOYxLH(0); 

    // Now to figure out what keys were newly pressed
    // XOR with previous frame, 
    // then AND with current frame
    input_pad0_new = ((temp_pad0 ^ input_pad0) & input_pad0);

    // The game only supports 1 pad
    /*uint16_t temp_pad1 = input_pad1; 

    input_pad1 = REG_JOYxLH(1); 
    input_pad1_new = ((temp_pad1 ^ input_pad1) & input_pad1);*/

    system_check_for_soft_reset(); // Place the soft reset check at the end of input polling
    
    if (input_pad0_new != 0 && !rand_seeded)
    {
        // Seed it now if it's still not seeded
        rand_seed(system_frames_elapsed);
    }
    
    return;
}

inline uint16_t system_check_for_key(enum KEYPAD_BITS k)
{
    if ((input_pad0_new & k) == k)
    {
        return 1;
    }

    return 0;
}

inline uint16_t system_check_for_key_hold(enum KEYPAD_BITS k)
{
    if ((input_pad0 & k) == k)
    {
        return 1;
    }

    return 0;
}

void system_interrupt_enable()
{
    // Set up interrupts
    register volatile uint8_t temp1 = REG_RDNMI;
    register volatile uint8_t temp2 = REG_TIMEUP;

    // Compiler bug has been worked around from ASM side.
    REG_NMITIMEN = (INT_VBLENABLE|INT_JOYPAD_ENABLE);
    
    emitCLI();

    return;
}

void system_interrupt_disable()
{
    // Set up interrupts
    register volatile uint8_t temp1 = REG_RDNMI;
    register volatile uint8_t temp2 = REG_TIMEUP;

    // Compiler bug has been worked around from ASM side.
    REG_NMITIMEN = INT_JOYPAD_ENABLE;
    
    emitSEI();

    return;
}

void system_reset()
{
    system_interrupt_disable();

    REG_INIDISP = 0x8f;

    #ifdef __VBCC__ // This should use the compiler define always
        __asm("\tjml $008000\n");
    #endif

    #ifdef __CALYPSI__ // This should use the compiler define always
        __asm("\tjmp long:0x008000\n");
    #endif

    return;
}

inline void system_check_for_soft_reset()
{
    if ((input_pad0 & (KEY_L | KEY_R | KEY_SELECT | KEY_START)) == (KEY_L | KEY_R | KEY_SELECT | KEY_START)) // check soft reset combo
    {
        if ((input_pad0 & 0x000f) == 0) // check signature
        {
            snd_reset(); // Reset the SPC too
            system_reset();
        }
    }

    return;
}