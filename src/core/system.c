#include "snes/console.h"

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
#include "loop_cutscene.h"
#include "loop_subscreen.h"
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
#include "ui_vwf.h"

#include "errorhandling.h"

#include "data_strings.h"

uint8_t system_MVNCodeInWRAM[4];

// Input system
uint16_t input_pad0;
uint16_t input_pad0_new;

/*
    VBCC doesn't initialize the zero page variables by default.

    Now using custom startup that does indeed init ZP
*/

/**
 * @brief Configures standard CPU registers and hardware states on startup.
 * 
 * Turns off the display (forced blank) and configures initial hardware registers.
 */
void System_Init_CpuRegs(void)
{
    // TODO: if to be released outside of SNES this will need to be #ifdef'd
    // Must do first
    REG_NMITIMEN = 0x00; // Disable interrupts
    REG_HDMAEN = 0x00; // Disable HDMA
    REG_INIDISP = 0x8f; // Disable the screen - stdio might turn it on

    REG_MEMSEL = 0x01; // Enable fastROM

    shadow_nmitimen = 0x00;
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

/**
 * @brief Dynamically writes assembly utility routines (such as block moves) to WRAM.
 */
void System_Init_WramFunctions()
{
    system_MVNCodeInWRAM[0] = 0x54;
    system_MVNCodeInWRAM[3] = 0x6b;

    return;
}

/**
 * @brief Displays the startup splash screen and initializes resources.
 * 
 * Performs core engine startup sequences, loads logo assets, and boots the SPC700 sound engine.
 */
void System_DisplayStartupSplash()
{
    // Set up the PPU regs to what we want.
    System_Init_BgScroll();

    ErrorHandler_Internal_Setup();

    ErrorHandler_Internal_Display((uint8_t *)&STR_STARTUP);

    // Set up a fade-in. Doing this so that we can actually run the other steps
    // while the game is still setting up.
    
    gfx_mosaic_change = -1;
    gfx_mosaic_layers = 0x01; // BG1
    gfx_mosaic_intensity = 0x0f00; // Max intensity
    system_use_alternate_nmi = true;

    shadow_brightness_change = (64 * V_MUL);
    shadow_brightness = 0 << 8;
    shadow_fblank_enable = 0x00;

    System_EnableInterrupts();

    // The SPC takes a while to init itself, so do something else in the meantime.

    // Check the SRAM contents
    Sram_Check();
    
    System_Init(); // Do the init here too
    
    // Assign the level pointers
    level_data_ptr = LEVEL_INITIAL; // Set the initial level here
    level_data_ptr_prev = LEVEL_INITIAL;
    level_data_ptr_next = LEVEL_INITIAL;

    //LevelSystem_LoadLevel(level_data_ptr); // non-VRAM hitting parts here
    
    while (shadow_brightness < (15 << 8))
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

    shadow_brightness_change = 0;

    gfx_mosaic_change = 0;
    system_use_alternate_nmi = false;

    while (shadow_brightness > 0)
    {
        while ((REG_HVBJOY & VBL_READY) != VBL_READY)
        {
            ;
        }

        REG_INIDISP = shadow_brightness >> 8;

        REG_MOSAIC = shadow_mosaic;

        shadow_mosaic = (((0x0f - (shadow_brightness >> 8)) << 4) | 0x01);

        while ((REG_HVBJOY & VBL_READY) == VBL_READY)
        {
            ;
        }

        shadow_brightness -= (64 * V_MUL);
    }

    shadow_mosaic = 0x00;

    REG_MOSAIC = shadow_mosaic;
    REG_INIDISP = 0x8f;
    shadow_brightness = 0;
    
    return;
}

/**
 * @brief Initializes graphics parameters, VRAM layouts, and regenerates tilemaps.
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

/**
 * @brief Constructs the static tilemap structure for the UI layers.
 */
void System_Init_UiTilemap()
{
    // Start with the shared initial items.
    REG_A1T0LH = (uint16_t)((uint32_t)&dma_filler_val);
    REG_A1B0 = (uint8_t)(((uint32_t)&dma_filler_val) >> 16);
    
    // BG1, high byte
    REG_DMAP0 = 0x08; // byte reg write, fixed increment

    REG_BBAD0 = 0x19; // VMDATAH
    REG_VMAIN = VRAM_INCHIGH;
    REG_VMADDLH = TILEMAP_ADDR_GAME_UI_4BPP;

    REG_DAS0LH = 1024;

    // See https://cppreference.com/c/language/volatile
    // on why this is needed.
    // 
    // Relevant excrept:
    //     A cast of a non-volatile value to a volatile type has no effect.
    //     To access a non-volatile object using volatile semantics, 
    //     its address must be cast to a pointer-to-volatile and then 
    //     the access must be made through that pointer.

    volatile uint8_t * fill = (uint8_t *)&dma_filler_val;
    *fill = 0x01;

    REG_MDMAEN = 0x01;

    // BG1, low byte
    REG_BBAD0 = 0x18; // VMDATAL
    REG_VMAIN = VRAM_INCLOW;
    REG_VMADDLH = TILEMAP_ADDR_GAME_UI_4BPP;

    REG_DAS0LH = 1024;

    *fill = 0x00;

    REG_MDMAEN = 0x01;

    // BG3, word
    REG_DMAP0 = 0x09; // word reg write, fixed increment

    REG_VMAIN = VRAM_INCHIGH;
    REG_VMADDLH = TILEMAP_ADDR_GAME_UI_2BPP;

    REG_DAS0LH = 2048;

    REG_MDMAEN = 0x01;

    return;
}


/**
 * @brief Performs a full cold reset initialization of the game state.
 * 
 * Clears standard objects, resets hitboxes, and resets game state variables.
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
    ObjectSystem_ResetStandardObjects(0); // The first time this is done, reset all objects
    ObjectSystem_ResetPlayerHitboxes(); // also reset hitbox list
    ObjectSystem_ResetEnemyHitboxes();
    Gfx_ResetSmoke();
    
    obj_player_index = -1;

    // Initialize BG scroll systems. Must be done before the map is loaded.
    bg_scroll_x_bounds_min.full.high.a = -32768;
    bg_scroll_y_bounds_min.full.high.a = -32768;
    bg_scroll_use_interpolation = false;

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

/**
 * @brief Performs a partial reset of the game state when reloading levels.
 * 
 * Resets standard objects and hitboxes but preserves the active player object state.
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
    ObjectSystem_ResetStandardObjects(1); // Reset all except player
    ObjectSystem_ResetPlayerHitboxes(); // also reset hitbox list
    ObjectSystem_ResetEnemyHitboxes();
    Gfx_ResetSmoke();
    
    // Initialize BG scroll systems. Must be done before the map is loaded.
    bg_scroll_x_bounds_min.full.high.a = -32768;
    bg_scroll_y_bounds_min.full.high.a = -32768;
    bg_scroll_use_interpolation = false;

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

/**
 * @brief Resets background scroll registers for layers BG1, BG2, and BG3.
 */
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

/**
 * @brief Sets display settings (brightness, layer enables) based on the game routine.
 * 
 * @param routine The active routine identifier (e.g. ROUTINE_GAMELOOP, ROUTINE_SUBSCREEN).
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
        case ROUTINE_CUTSCENE:
        case ROUTINE_CUTSCENE_INIT:
            REG_BGMODE = 0x01; // Mode 1
            REG_TM = TM_CS; // BG1 and OBJ
            break;
        case ROUTINE_MAPDISPLAY:
            REG_BGMODE = 0x03; // Mode 3
            REG_TM = TM_MODE3; // BG1, BG2, and OBJ
            break;
    }

    return;
}

/**
 * @brief Sets up screen sizes and tilemap register configurations.
 * 
 * @param routine The active routine identifier.
 */
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
        case ROUTINE_CUTSCENE:
        case ROUTINE_CUTSCENE_INIT:
            if (cs_use_second_frame)
            {
                REG_BG12NBA = 0 << 4 | 3;
                REG_BG1SC = TILEMAP_ADDR_CS_FRAME_B >> 8;
            }
            else
            {
                REG_BG12NBA = 0 << 4 | 0;
                REG_BG1SC = TILEMAP_ADDR_CS_FRAME_A >> 8;
            }
            break;
        case ROUTINE_MAPDISPLAY:
            REG_BG12NBA = 4 << 4 | 0;

            REG_BG1SC = TILEMAP_ADDR_MAP_MAP >> 8;
            REG_BG2SC = TILEMAP_ADDR_MAP_UI >> 8;
            break;
    }

    return;
}

/**
 * @brief Blocks execution until VBlank, performing input polling and soft reset checks.
 */
void System_WaitUntilVblank()
{
    System_CheckForActiveDisplayEnd();

    system_in_vblank = true; // This must be the last value written.

    while (system_in_vblank)
    {
        Asm_EmitWai();
    }  

    if (system_use_long_vblank)
    {
        System_GetInput_Manual();
    }
    else
    {
        System_GetInput();
    }

    System_CheckSoftReset(); // Place the soft reset check at the end of input polling
    
    if (input_pad0_new != 0 && !rand_seeded)
    {
        // Seed it now if it's still not seeded
        Math_SeedRandom(system_frames_elapsed);
    }
        
    return;
}

// NOTE: the game polls both controllers and merges inputs.

/**
 * @brief Polls controller buttons via hardware auto-joypad read registers.
 */
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
            "\tora $421a\n"
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
        input_pad0 = REG_JOYxLH(0) | REG_JOYxLH(1); // Read both and merge

        // Now to figure out what keys were newly pressed
        // XOR with previous frame, 
        // then AND with current frame
        input_pad0_new = ((temp_pad0 ^ input_pad0) & input_pad0);
    #endif
    
    return;
}

/**
 * @brief Manually clocks and reads SNES controller data (used for startup verification or when V-IRQ is used instead of standard NMI).
 */
void System_GetInput_Manual()
{
    #if VBCC_ASM == 1 // Don't bother changing memory bit
        __asm(
            "\ta8\n"
            "\tx16\n"
            "\tsep #$20\n"
            
            "\tlda #$01\n"
            "\tsta $4016\n"
            "\tstz $4016\n"

            "\tsta r1\n"
            "\tstz r1+1\n"
            "\tsta r2\n"
            "\tstz r2+1\n"

            ".input_read_1:\n"
            "\tlda $4016\n"
            "\tlsr\n"
            "\trol r1\n"
            "\trol r1+1\n"
            "\tbcc .input_read_1\n"

            ".input_read_2:\n"
            "\tlda $4017\n"
            "\tlsr\n"
            "\trol r2\n"
            "\trol r2+1\n"
            "\tbcc .input_read_2\n"

            "\ta16\n"
            "\trep #$20\n"

            "\tlda _input_pad0\n"
            "\tsta r0\n"
            "\tlda r1\n"
            "\tora r2\n"
            "\tsta _input_pad0\n"
            "\teor r0\n"
            "\tand _input_pad0\n"
            "\tsta _input_pad0_new\n"
        );
    #else
        // Reset controllers
        REG_JOYOUT = 0x01;
        REG_JOYOUT = 0x00;

        // The controller is ready to be read.
        uint16_t controller_bits_0 = 0x0000;
        uint16_t controller_bits_1 = 0x0000;
        uint8_t bit = 0x00; // Read and paste controller data here.

        // Controller 1
        for (int i = 0; i < 16; i++) // 16 bits in standard controller
        {
            bit = REG_JOYSERx(0);
            bit = bit & 0x01;

            controller_bits_0 = (controller_bits_0 << 1) | bit;
        }

        // Controller 2
        for (int i = 0; i < 16; i++) // 16 bits in standard controller
        {
            bit = REG_JOYSERx(1);
            bit = bit & 0x01;

            controller_bits_1 = (controller_bits_1 << 1) | bit;
        }

        uint16_t temp_pad0 = input_pad0; 
        input_pad0 = controller_bits_0 | controller_bits_1;
        input_pad0_new = ((temp_pad0 ^ input_pad0) & input_pad0);
    #endif

    return;
}

/**
 * @brief Checks and returns the controller ID signature to verify joypad validity.
 * 
 * @return Joypad ID signature (e.g., 0 for a standard controller).
 */
uint16_t System_CheckController(void)
{
    System_GetInput_Manual();

    return (input_pad0 & 0x0f);
}

/**
 * @brief Checks if a specific button was newly pressed in the current frame.
 * 
 * @param k The keypad bit flag to query.
 * @return Non-zero if the key was newly pressed; otherwise zero.
 */
uint16_t System_CheckKey(enum KEYPAD_BITS k)
{
    if (subscreen_transition_state != 0)
    {
        return 0;
    }
    if ((input_pad0_new & k) == k)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief Checks if any button on the joypad was newly pressed in the current frame.
 * 
 * @return Non-zero if any button was newly pressed; otherwise zero.
 */
uint16_t System_CheckKeyAny()
{
    if (subscreen_transition_state != 0)
    {
        return 0;
    }
    if (input_pad0_new != 0x0000)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief Checks if a specific button is currently held down.
 * 
 * @param k The keypad bit flag to query.
 * @return Non-zero if the key is currently held; otherwise zero.
 */
uint16_t System_CheckKeyHeld(enum KEYPAD_BITS k)
{
    if (subscreen_transition_state != 0)
    {
        return 0;
    }
    if ((input_pad0 & k) == k)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief Enables VBlank interrupts (NMI) and hardware polling.
 */
void System_EnableInterrupts()
{
    system_use_long_vblank = false;

    // Set up interrupts
    register volatile uint8_t temp1 = REG_RDNMI;
    register volatile uint8_t temp2 = REG_TIMEUP;

    shadow_nmitimen = INT_VBLENABLE | INT_JOYPAD_ENABLE;
    REG_NMITIMEN = INT_VBLENABLE | INT_JOYPAD_ENABLE;
    
    Asm_EmitCli();

    return;
}

/**
 * @brief Disables hardware interrupts (NMI).
 */
void System_DisableInterrupts()
{
    system_use_long_vblank = false;

    // Set up interrupts
    register volatile uint8_t temp1 = REG_RDNMI;
    register volatile uint8_t temp2 = REG_TIMEUP;

    shadow_nmitimen = 0x00;
    REG_NMITIMEN = 0x00;
    
    Asm_EmitSei();

    return;
}

/**
 * @brief Enables vertical interrupts (V-IRQ) which allows running the Vblank routine earlier than the standard line.
 */
void System_EnableFblankInterrupts()
{
    system_use_long_vblank = true;

    // Set up interrupts
    register volatile uint8_t temp1 = REG_RDNMI;
    register volatile uint8_t temp2 = REG_TIMEUP;

    REG_VTIMELH = 209; // Fblank at line 209

    // Compiler bug has been worked around from ASM side.
    shadow_nmitimen = INT_HVIRQ_V;
    REG_NMITIMEN = INT_HVIRQ_V;
    
    Asm_EmitCli();

    return;
}

/**
 * @brief Performs a hard system reset sequence.
 */
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

/**
 * @brief Checks if the L+R+Start+Select soft reset combination is held.
 */
void System_CheckSoftReset()
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

/**
 * @brief Soft resets the game, resetting the APU sound engine and returning to splash.
 */
void System_SoftReset()
{
    if (snd_apu_booted)
    {
        SoundInterface_ResetAPU(); // Reset the SPC too
    }
    
    System_Reset();

    return;
}

/**
 * @brief Synchronizes code execution by blocking until the start of VBlank.
 */
void System_AlignToVblank()
{
    #if VBCC_ASM == 1
        __asm(
            "\ta8\n"
            "\tsep #$20\n"
            "\tbit $4212\n"
            "\tbpl .phase_2\n" // Not in hblank
            "\t.phase_1:\n"
            "\tbit $4212\n"
            "\tbmi .phase_1\n" // Still in hblank
            "\t.phase_2:\n"
            "\tbit $4212\n"
            "\tbpl .phase_2\n" // Not in hblank
            "\ta16\n"
            "\trep #$20\n"
            );
    #else
        while ((REG_HVBJOY & VBL_READY) == VBL_READY)
        {
            ;
        }
        while ((REG_HVBJOY & VBL_READY) != VBL_READY)
        {
            ;
        }
    #endif

    return;
}

/**
 * @brief Increments system frame counters and update timing structures.
 */
void System_UpdateFrameCounters()
{
    system_frames_elapsed++;

    system_time_subframe++;
    if (system_time_subframe >= FPS)
    {
        system_time_subframe = 0;
        system_time_s++;

        if (system_time_s >= 60)
        {
            system_time_s = 0;
            system_time_m++;

            if (system_time_m >= 60)
            {
                system_time_m = 0;
                system_time_h++;
            }
        }
    }

    return;
}

#if VBCC_ASM == 1
/**
 * @brief Inserts a custom delay loop aligned to horizontal sync (H-sync) lines.
 * 
 * @param dot The line offset constraint.
 */
NO_INLINE void System_Hsync(uint16_t dot)
#else 
/**
 * @brief Inserts a custom delay loop aligned to horizontal sync (H-sync) lines.
 * 
 * @param dot The line offset constraint.
 */
void System_Hsync(uint16_t dot)
#endif
{
    #if VBCC_ASM == 1
        __asm(
            "\ta16\n"
            "\tsta $4207\n"
            
            "\ta8\n"
            "\tsep #$24\n"

            "\tlda _shadow_nmitimen\n"
            "\tora #$10\n"
            "\tsta $4200\n"

            "\twai\n"

            "\tbit $4211\n"
            
            "\tlda _shadow_nmitimen\n"
            "\tsta $4200\n"

            "\ta16\n"
            "\trep #$24\n"
            );
    #else
        Asm_EmitSei();
        REG_HTIMELH = dot;
        register volatile uint8_t temp = REG_RDNMI;
        temp = REG_TIMEUP;

        REG_NMITIMEN = shadow_nmitimen | INT_HVIRQ_H;
        
        Asm_EmitWai();
        temp = REG_RDNMI;
        temp = REG_TIMEUP;

        REG_NMITIMEN = shadow_nmitimen;
        Asm_EmitCli();
    #endif

    return;
}

/**
 * @brief Checks if the active display beam has ended for VBlank sync checks.
 */
void System_CheckForActiveDisplayEnd()
{
    #if VBCC_ASM == 1
        __asm(
            "\ta8\n"
            "\tsep #$20\n"
            "\tbit $2137\n"
            
            "\tlda $213d\n"
            "\tsta r0\n"
            "\tbit $213d\n" // We don't really need to check the entire thing, just the low byte. So just read this to make sure both values are read.

            "\tlda _system_use_long_vblank\n"
            "\tbeq .normal\n" 

            "\tlda r0\n"
            "\tcmp #208\n"
            "\tbne .end\n"
            ".fblank_loop:"  // Line 208
                "\tbit $213f\n"
                "\tbit $2137\n"
                "\tlda $213d\n"
                "\tbit $213d\n"
                "\tcmp #208\n"
                "\tbeq .fblank_loop\n"
                "\tcmp #209\n"
                "\tbeq .fblank_loop\n"
                "\tbra .end\n"

            ".normal:\n" // Line 224
            "\tlda r0\n"
            "\tcmp #224\n"
            "\tbne .end\n"

            ".normal_loop:"
                "\tbit $213f\n"
                "\tbit $2137\n"
                "\tlda $213d\n"
                "\tbit $213d\n"
                "\tcmp #224\n"
                "\tbeq .normal_loop\n"
                "\tcmp #225\n"
                "\tbeq .normal_loop\n"

            "\t.end:\n"

            "\tbit $213f\n"
            "\ta16\n"
            "\trep #$20\n"
        );
    #else
        // Check if the current scanline is exactly 224 or not.
        register volatile uint8_t temp = REG_SLHV;
        uint8_t scanline_lo = REG_OPVCT;
        uint8_t scanline_hi = REG_OPVCT & 0x01;
        uint16_t scanline = scanline_lo | (scanline_hi << 8);

        uint16_t target_line = 224; // Last line of active display

        if (system_use_long_vblank)
        {
            target_line = 208;
        }

        if (scanline == target_line)
        {
            while ((scanline == target_line) || (scanline == (target_line+1)))
            {
                temp = REG_STAT78;
                temp = REG_SLHV;
                scanline_lo = REG_OPVCT;
                scanline_hi = REG_OPVCT & 0x01;
                scanline = scanline_lo | (scanline_hi << 8);
            }
        }

        temp = REG_STAT78;
    #endif

    return;
}

/**
 * @brief Quick memory block copy utility.
 * 
 * @param src  [r0/r1] Source memory address pointer.
 * @param dest [r2/r3] Destination memory address pointer.
 * @param len  [a] Number of bytes to copy. A length of 0 bytes is equal to 65,536 bytes.
 */
#if VBCC_ASM == 1
    NO_INLINE void System_CopyBlock(__reg("r0/r1") uint8_t * src, __reg("r2/r3") uint8_t * dest, __reg("a") uint16_t len)
#else
    void System_CopyBlock(uint8_t * src, uint8_t * dest, uint16_t len)
#endif
{
    // r0 contains source
    // r2 contains destination
    // a contains bytes to copy
    #if VBCC_ASM == 1
        __asm(
        "\ta16\n"
	    "\tx16\n"

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
