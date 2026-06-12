#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

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
#include "sram_management.h"
#include "level.h"
#include "gfx.h"

#include "ani.h"
#include "ani_bg.h"
#include "ani_pal.h"
#include "hdma.h"

#include "snd.h"
#include "ui.h"

uint8_t system_MVNCodeInWRAM[4];
uint8_t system_JMLCodeInWRAM[4];

// Input system
uint16_t input_pad0;
uint16_t input_pad0_new;

/*
    VBCC doesn't initialize the zero page variables by default.

    Now using custom startup that does indeed init ZP
*/

/*
    Initialize all system registers. In case something else was running beforehand

    TODO: if to be released outside of SNES this will need to be #ifdef'd
*/
void System_Init_CpuRegs(void)
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

    System_Init_BgScroll();

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

// Write out the MVN and JML opcodes here so that they can be used ASAP
void System_Init_WramFunctions(void)
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

    return;
}

/*
    Throw a splash screen during initialization to prevent extended black screens
    Put as many init parts as possible here
*/
void System_DisplayStartupSplash()
{
    // Set up the PPU regs to what we want.
    System_Init_BgScroll();

    REG_BGMODE = 0x09; // Mode 1, high priority bg3
    REG_TM = 0x01; // BG1 only
    REG_BG12NBA = 0 << 4 | 0;

    REG_BG1SC = 0x3800 >> 8; // The image is 28KB. Have the tilemap go to the 28Kth byte.

    // Decompress the splash and tilemap
    LZ4_UnpackToWRAM((void *)&data_bg_splash_lz4, 0x007f0000);
    LZ4_UnpackToWRAM((void *)&data_tilemap_splash_lz4, 0x007f7000);
    
    // Copy the palette
    DmaSystem_CopyToWram((uint32_t)data_palette_splash, (uint32_t)&shadow_cgram, 256);

    // Upload the splash
    DmaSystem_CopyToVram(0x007f0000, 0x0000, 0x7800); // Copy the entire section including the tilemap.
    DmaSystem_UploadCgram();

    // Set up a fade-in. Doing this so that we can actually run the other steps
    // while the game is still setting up.
    shadow_inidisp_change = 1;
    gfx_mosaic_change = -1;
    gfx_mosaic_layers = 0x01; // BG1
    gfx_mosaic_intensity = 16; // Max intensity
    system_use_alternate_nmi = 1;

    shadow_inidisp = 0x00;

    System_EnableInterrupts();

    // Check the SRAM contents
    sram_check();

    // The SPC takes a while to init itself, so do something else in the meantime.
    System_Init(); // Do the init here too
    
    // Load the level
    level_data_ptr = LEVEL_INITIAL; // Set the initial level here
    level_data_ptr_prev = LEVEL_INITIAL;

    LevelSystem_LoadLevel(level_data_ptr); // non-VRAM hitting parts here
    
    while (shadow_inidisp != 0x0f)
    {
        ; // Prevent execution from continuing to SPC upload while the screen isn't fully bright
    }

    // In case the above initialization take too short this should prevent issues
    System_DisableInterrupts(); // uploading the SPC while interrupts are on can cause lock-ups

    snd_current_command_counter = 0;
    SoundInterface_StartSoundEngine(); // start the SPC

    // Upload instrument and music sequence data
    // TODO: describe a sequence pointer and structure so this can be handled as a single pointer to pass to a function
    // Upload SFX data (shared for entire game)
    SoundInterface_UploadSampleList((struct sample_list_entry *)&data_snd_samples[0]);
    SoundInterface_UploadInstrumentList((struct sample_list_entry_ins *)&data_snd_instruments[0]);
    SoundInterface_UploadMusicSequence((struct seq_command *)&data_seq_test_t1[0], 0); // Drum 1
    SoundInterface_UploadMusicSequence((struct seq_command *)&data_seq_test_t2[0], 1); // Drum 2
    SoundInterface_UploadMusicSequence((struct seq_command *)&data_seq_test_t3[0], 2); // Bass
    SoundInterface_UploadMusicSequence((struct seq_command *)&data_seq_test_t4[0], 3); // Secondary
    //SoundInterface_UploadMusicSequence((struct seq_command *)&data_seq_test_t5[0], 4); // Drum test sequence
    //SoundInterface_UploadMusicSequence((struct seq_command *)&data_seq_test_t6[0], 5); // Drum + instrument test sequence
    SoundInterface_SetMusicTempo(120);

    shadow_inidisp_change = 0;
    gfx_mosaic_change = 0;
    system_use_alternate_nmi = 0;

    while (shadow_inidisp != 0x00)
    {
        while ((REG_HVBJOY & VBL_READY) != VBL_READY)
        {
            ;
        }

        REG_INIDISP = shadow_inidisp;

        REG_MOSAIC = shadow_mosaic;

        shadow_mosaic = (((0x0f - shadow_inidisp) << 4) | 0x01);

        while ((REG_HVBJOY & VBL_READY) == VBL_READY)
        {
            ;
        }

        shadow_inidisp -= 1;
    }

    shadow_mosaic = 0x00;

    REG_MOSAIC = shadow_mosaic;
    REG_INIDISP = 0x8f;
    shadow_inidisp = 0;

    // Screen is forced blank again. Do anything that touches PPU regs here now

    // DMA graphics in its entirety
    DmaSystem_CopyToVram(0x007f0000, 0x0000, 0);

    // Initialize global DMA tile animation
    // TODO: currently hardcoded. In the future, pointers may be part of map data.
    buf_player_prev_frame = 0xffff; // Write an invalid frame here

    AniSystem_BgTile_Setup((uint8_t *)&data_bg_dungeon_anim_water_lz4, (uint8_t *)&data_bg_dungeon_anim_torch_lz4);

    // Finish initializing graphics
    System_Init_Graphics();

    // Run one frame of partial game logic to draw sprites
    Loop_Game_Partial();
    
    return;
}

/*
    Initialization of fixed sprites that touch VRAM must be done in fblank
    Also for safety, PPU registers that are touched also go here
*/
void System_Init_Graphics(void)
{
    // Write BG1, BG2, BG3 and BG4 scroll to 0 on X and negative 1 on Y axis
    System_Init_BgScroll();

    // Set up sprite display
    REG_OBSEL = OBJ_SIZE16_L32|3;

    // Regenerate the tilemaps
    MapSystem_Tilemap_RegenerateTilemap();

    System_Init_UiTilemap();

    return;
}

void System_Init_UiTilemap()
{
    // flush the BG1 tilemap with the correct null tiles
    #if VBCC_ASM == 1
        REG_VMAIN = VRAM_INCLOW;
        REG_VMADDLH = TILEMAP_ADDR_GAME_UI_4BPP;

        __asm(
            "\ta8\n"
            "\tx16\n"
            "\tsep #$20\n"

            "\tldx #256\n"
            "\tstx r0\n"

            "\tlda #$08\n"
            "\tsta $4300\n"
            
            "\tldx #<r0\n"
            "\tstx $4302\n"
            "\tlda #^r0\n"
            "\tsta $4304\n"

            "\tldx #1024\n"
            "\tstx $4305\n"

            "\tlda #$18\n"
            "\tsta $4301\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );

        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_GAME_UI_4BPP;

        __asm(
            "\ta8\n"
            "\tx16\n"
            "\tsep #$20\n"

            "\tldx #256\n"
            "\tstx r0\n"

            "\tlda #$08\n"
            "\tsta $4300\n"
            
            "\tldx #<r0+1\n"
            "\tstx $4302\n"
            "\tlda #^r0\n"
            "\tsta $4304\n"

            "\tldx #1024\n"
            "\tstx $4305\n"

            "\tlda #$19\n"
            "\tsta $4301\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_GAME_UI_4BPP;

        for (int i = 0; i < 1024; i++)
        {
            REG_VMDATALH = 256;
        }
    #endif

    // Repeat for BG3
    #if VBCC_ASM == 1
        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_GAME_UI_2BPP;

        __asm(
            "\ta8\n"
            "\tx16\n"
            "\tsep #$20\n"

            "\tldx #$00000\n"
            "\tstx r0\n"

            "\tlda #$09\n"
            "\tsta $4300\n"
            
            "\tldx #<r0\n"
            "\tstx $4302\n"
            "\tlda #^r0\n"
            "\tsta $4304\n"

            "\tldx #2048\n"
            "\tstx $4305\n"

            "\tlda #$18\n"
            "\tsta $4301\n"

            "\tlda #$01\n"
            "\tsta $420b\n"

            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        REG_VMAIN = VRAM_INCHIGH;
        REG_VMADDLH = TILEMAP_ADDR_GAME_UI_2BPP;

        for (int i = 0; i < 1024; i++)
        {
            REG_VMDATALH = 0;
        }
    #endif

    return;
}


/*
    First part of initialization that doesn't touch graphics (can be done during the splash screen)
*/
void System_Init()
{
    // Initialize VRAM slot allocator
    SpriteEngine_InitVramSlot();
    
    // Reset sprites
    spr_sprite_count_prev = 128;
    spr_sprite_count = 0;
    SpriteEngine_ResetOam();
    SpriteEngine_PackOamHighTable();

    // Reset object system
    obj_reset(0); // The first time this is done, reset all objects
    obj_reset_hitbox_player(); // also reset hitbox list
    obj_reset_hitbox_enemy();
    
    obj_player_index = -1;

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

    return;
}

/*
    Call during a live game
*/
void System_Init_Partial()
{
    // Initialize VRAM slot allocator
    SpriteEngine_InitVramSlot();
    
    // Reset sprites
    spr_sprite_count_prev = 128;
    spr_sprite_count = 0;
    SpriteEngine_ResetOam();
    SpriteEngine_PackOamHighTable();

    // Reset object system
    obj_reset(1); // Reset all except player
    obj_reset_hitbox_player(); // also reset hitbox list
    obj_reset_hitbox_enemy();
    
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

    // Reset enemy counter
    obj_enemies_defeated = 0;
    obj_enemies_target_count = 0;
    obj_enemies_max_count = 0;

    return;
}

void System_Init_BgScroll(void)
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
void System_Init_DisplaySettings(uint16_t routine)
{
    switch (routine)
    {
        case ROUTINE_GAMELOOP:
        case ROUTINE_SUBSCREEN:
        case ROUTINE_SUBSCREEN_HELP:
        case ROUTINE_MSGBOX:
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
void System_Init_TilemapSettings(uint16_t routine)
{
    switch (routine)
    {
        case ROUTINE_GAMELOOP:
        case ROUTINE_SUBSCREEN:
        case ROUTINE_SUBSCREEN_HELP:
        case ROUTINE_MSGBOX:
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

FORCE_INLINE void System_WaitUntilVblank()
{
    // A workaround has been done on the ASM code side
    system_in_vblank = 1; // This must be the last value written.

    while (system_in_vblank)
    {
        emitWAI();
    }  

    System_GetInput();
        
    return;
}

void System_GetInput()
{
    #if VBCC_ASM == 1 // Don't bother changing memory bit
        __asm(
            "\ta16\n"
            "\tx16\n"
            
            ".input_wait:\n"
            "\tlda $4212\n"
            "\tand #$01\n"
            "\tbne .input_wait\n"
            "\tlda _input_pad0\n"
            "\tsta r0\n"
            "\tlda $4218\n"
            "\tsta _input_pad0\n"
            "\teor r0\n"
            "\tand _input_pad0\n"
            "\tsta _input_pad0_new\n"
        );
    #else
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
    #endif

    System_CheckSoftReset(); // Place the soft reset check at the end of input polling
    
    if (input_pad0_new != 0 && !rand_seeded)
    {
        // Seed it now if it's still not seeded
        Math_SeedRandom(system_frames_elapsed);
    }
    
    return;
}

FORCE_INLINE uint16_t System_CheckKey(enum KEYPAD_BITS k)
{
    if ((input_pad0_new & k) == k)
    {
        return 1;
    }

    return 0;
}

FORCE_INLINE uint16_t System_CheckKeyAny()
{
    if (input_pad0_new != 0x0000)
    {
        return 1;
    }

    return 0;
}

FORCE_INLINE uint16_t System_CheckKeyHeld(enum KEYPAD_BITS k)
{
    if ((input_pad0 & k) == k)
    {
        return 1;
    }

    return 0;
}

FORCE_INLINE void System_EnableInterrupts()
{
    // Set up interrupts
    register volatile uint8_t temp1 = REG_RDNMI;
    register volatile uint8_t temp2 = REG_TIMEUP;

    // Compiler bug has been worked around from ASM side.
    REG_NMITIMEN = (INT_VBLENABLE|INT_JOYPAD_ENABLE);
    
    emitCLI();

    return;
}

FORCE_INLINE void System_DisableInterrupts()
{
    // Set up interrupts
    register volatile uint8_t temp1 = REG_RDNMI;
    register volatile uint8_t temp2 = REG_TIMEUP;

    // Compiler bug has been worked around from ASM side.
    REG_NMITIMEN = INT_JOYPAD_ENABLE;
    
    emitSEI();

    return;
}

void System_Reset()
{
    System_DisableInterrupts();

    REG_INIDISP = 0x8f;

    #ifdef __VBCC__ // This should use the compiler define always
        __asm("\tjml $008000\n");
    #endif

    #ifdef __CALYPSI__ // This should use the compiler define always
        __asm("\tjmp long:0x008000\n");
    #endif

    return;
}

FORCE_INLINE void System_CheckSoftReset()
{
    if ((input_pad0 & (KEY_L | KEY_R | KEY_SELECT | KEY_START)) == (KEY_L | KEY_R | KEY_SELECT | KEY_START)) // check soft reset combo
    {
        if ((input_pad0 & 0x000f) == 0) // check signature
        {
            System_SoftReset();
        }
    }

    return;
}

void System_SoftReset()
{
    SoundInterface_ResetAPU(); // Reset the SPC too
    System_Reset();

    return;
}

// Call this to align to vblank
void System_AlignToVblank()
{
    while ((REG_HVBJOY & VBL_READY) != VBL_READY)
    {
        ;
    }
    while ((REG_HVBJOY & VBL_READY) == VBL_READY)
    {
        ;
    }
    while ((REG_HVBJOY & VBL_READY) != VBL_READY)
    {
        ;
    }

    return;
}

#if VBCC_ASM == 1
NO_INLINE void System_CopyBlock(__reg("r0/r1") uint8_t * src, __reg("r2/r3") uint8_t * dest, __reg("a") uint16_t len)
#else
FORCE_INLINE void System_CopyBlock(uint8_t * src, uint8_t * dest, uint16_t len)
#endif
{
    // r0 contains source
    // r2 contains destination
    // a contains bytes to copy
    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
	    "\tx16\n"
        
        "\tcmp #0\n"
        "\tbeq LZ4_Internal_Skip\n"

        "\tphb\n"

        "\ttax\n"
        "\ta8\n"
        "\tsep #$20\n"
        "\tlda r3\n"
        "\tsta >_system_MVNCodeInWRAM+1\n" // write bank byte of source 
        "\tlda r1\n"
        "\tsta >_system_MVNCodeInWRAM+2\n" // ditto for destination
        "\ta16\n"
        "\trep #$20\n"
        "\ttxa\n"
        "\tdec\n"
        "\tldx r0\n"
        "\tldy r2\n"
        "\tjsl >_system_MVNCodeInWRAM;\n"

        "\tplb\n"
        "LZ4_Internal_Skip:\n"
        );
    #else
    // Source and destination bank independent, just can't cross banks
    for (uint16_t i = 0; i < len; i++)
    {
        (*dest++) = (*src++);
    }
    #endif

    return;
}
