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
#include "spr.h"

#include "snd.h"
#include "ui.h"
#include "ui_vwf.h"
#include "ui_textscreen.h"

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

    REG_CGWSEL = CM_NEVER_ENABLE;
    REG_CGADSUB = 0;
    REG_COLDATA = CM_APPLY_BLUE | CM_APPLY_GREEN | CM_APPLY_RED;

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

/*
    Initializes user-settable settings for audio and graphics.
*/
void System_Init_Settings()
{
    snd_settings_volume = SND_DEFAULT_VOLUME;
    snd_settings_enable_bgm = true;
    snd_settings_enable_sfx = true;
    snd_settings_enable_voice = true;

    gfx_enable_hitblur = true;
    gfx_enable_heatwave = true;

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

    // Reset the sprites
    SpriteEngine_ResetSpriteLists();
    
    DmaSystem_UploadOam();
    REG_OBSEL = OBJ_SIZE16_L32|3;

    // Upload the title screen option graphics and palette
    AniSystem_Pal_LoadSubpalette((uint8_t *)&data_palette_title_options, 8); // Upload title options palette
    LZ4_UnpackToVRAM((void *)&data_spr_title_options_lz4, 0x6000); // Upload title options graphics

    Ui_TextScreen_Setup();

    Ui_TextScreen_Display((uint8_t *)&STR_STARTUP);

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

    // Initialize user game settings
    System_Init_Settings();
    
    SoundInterface_SetMasterVolume(snd_settings_volume);

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
    REG_A1T0LH = ADDR_LOWORD(&dma_filler_val);
    REG_A1B0 = ADDR_BANK(&dma_filler_val);
    
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
        case ROUTINE_TITLE:
        case ROUTINE_TITLE_INIT:
            REG_BGMODE = 0x09; // Mode 1, high priority bg3
            REG_TM = TM_TITLE; // BG1 and OBJ
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
        case ROUTINE_TITLE:
        case ROUTINE_TITLE_INIT:
            REG_BG12NBA = 0 << 4 | 0;

            REG_BG1SC = 0x3800 >> 8;
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

// Extracted to src/core/system_opt.asm and src/core/system_opt.c.

// Extracted to src/core/system_opt.asm and src/core/system_opt.c.

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

// Extracted to src/core/system_opt.asm and src/core/system_opt.c.

#if VBCC_ASM == 0
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
#endif


static size_t format_u32_dec(uint32_t val, char *buf, size_t maxlen, int min_width, bool zero_pad)
{
    char tmp[11];
    int len = 0;

    if (val == 0)
    {
        tmp[len++] = '0';
    }
    else
    {
        while (val > 0)
        {
            tmp[len++] = '0' + (val % 10);
            val /= 10;
        }
    }

    int padding = (min_width > len) ? (min_width - len) : 0;
    char pad_char = zero_pad ? '0' : ' ';
    size_t written = 0;

    while (padding > 0 && written + 1 < maxlen)
    {
        buf[written++] = pad_char;
        padding--;
    }

    while (len > 0 && written + 1 < maxlen)
    {
        buf[written++] = tmp[--len];
    }

    return written;
}

static size_t format_u32_hex(uint32_t val, char *buf, size_t maxlen, int min_width, bool zero_pad, bool uppercase)
{
    static const char hex_lower[] = "0123456789abcdef";
    static const char hex_upper[] = "0123456789ABCDEF";
    const char *hex_digits = uppercase ? hex_upper : hex_lower;

    char tmp[9];
    int len = 0;

    if (val == 0)
    {
        tmp[len++] = '0';
    }
    else
    {
        while (val > 0)
        {
            tmp[len++] = hex_digits[val & 0x0f];
            val >>= 4;
        }
    }

    int padding = (min_width > len) ? (min_width - len) : 0;
    char pad_char = zero_pad ? '0' : ' ';
    size_t written = 0;

    while (padding > 0 && written + 1 < maxlen)
    {
        buf[written++] = pad_char;
        padding--;
    }

    while (len > 0 && written + 1 < maxlen)
    {
        buf[written++] = tmp[--len];
    }

    return written;
}

/**
 * @brief Lightweight, integer-only vsnprintf implementation for SNES.
 */
int vsnprintf(char *str, size_t size, const char *format, va_list args)
{
    if (!str || size == 0) return 0;

    size_t out = 0;
    const char *p = format;

    while (*p && out + 1 < size)
    {
        if (*p != '%')
        {
            str[out++] = *p++;
            continue;
        }

        p++; // Skip '%'

        if (*p == '%')
        {
            str[out++] = '%';
            p++;
            continue;
        }

        bool zero_pad = false;
        if (*p == '0')
        {
            zero_pad = true;
            p++;
        }

        int min_width = 0;
        while (*p >= '0' && *p <= '9')
        {
            min_width = min_width * 10 + (*p - '0');
            p++;
        }

        bool is_long = false;
        if (*p == 'l')
        {
            is_long = true;
            p++;
        }

        char spec = *p++;
        switch (spec)
        {
            case 'c':
            {
                char c = (char)va_arg(args, int);
                str[out++] = c;
                break;
            }
            case 's':
            {
                const char *s = va_arg(args, const char *);
                if (!s) s = "(null)";
                while (*s && out + 1 < size)
                {
                    str[out++] = *s++;
                }
                break;
            }
            case 'd':
            case 'i':
            {
                int32_t val = is_long ? va_arg(args, int32_t) : (int32_t)va_arg(args, int);
                if (val < 0)
                {
                    str[out++] = '-';
                    val = -val;
                    if (min_width > 0) min_width--;
                }
                out += format_u32_dec((uint32_t)val, str + out, size - out, min_width, zero_pad);
                break;
            }
            case 'u':
            {
                uint32_t val = is_long ? va_arg(args, uint32_t) : (uint32_t)va_arg(args, unsigned int);
                out += format_u32_dec(val, str + out, size - out, min_width, zero_pad);
                break;
            }
            case 'x':
            case 'X':
            {
                uint32_t val = is_long ? va_arg(args, uint32_t) : (uint32_t)va_arg(args, unsigned int);
                out += format_u32_hex(val, str + out, size - out, min_width, zero_pad, (spec == 'X'));
                break;
            }
            default:
            {
                str[out++] = '%';
                if (spec) str[out++] = spec;
                break;
            }
        }
    }

    str[out] = '\0';
    return (int)out;
}

/**
 * @brief Lightweight, integer-only snprintf replacement.
 */
int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int res = vsnprintf(str, size, format, args);
    va_end(args);
    return res;
}
