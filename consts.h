#ifndef _WIN32
    #define INLINE_ASM 1
#endif

#ifdef __VBCC__
    #define NEAR __near
    #define ZP __zpage
    #define NO_INLINE __noinline
    #define INTERRUPT __interrupt
    #define CPU_65816 1
    #define VBCC_ASM INLINE_ASM
#endif

#ifdef __CALYPSI__
    #define NEAR 
    #define ZP 
    #define NO_INLINE 
    #define INTERRUPT
    #define CPU_65816 1
    #define CALYPSI_ASM INLINE_ASM
#endif

#if CPU_65816 != 1
    #define NEAR
    #define ZP
    #define NO_INLINE 
    #define INTERRUPT
#endif

// SRAM defines
#define SRAM_TOTAL_SIZE 131072l
#define SRAM_BANK_SIZE 8192l
#define SRAM_BANKS (SRAM_TOTAL_SIZE / SRAM_BANK_SIZE)
#define SRAM_ADDR 0x00306000
#define SRAM_DATA_OFFSET 0x0010 // after the checksum and reserved area

// Event flags
#define EVENT_FLAG_LOCAL_MAX 256
#define EVENT_FLAG_GLOBAL_MAX 256

// Frame rate and velocity defines
#define FPS 30
#define V_MUL (int)(60 / (int)FPS)
#define NMI_WAIT_COUNT V_MUL
#define V_P_ONE (1 * V_MUL)
#define V_S_ONE (65536l * V_MUL)
#define V_S_DIAGONAL (46341l * V_MUL)

#define V_GRAVITY V_S_ONE

// Animation defines (tied to frame rate, all powers of 2)
#define ANI_INTERVAL_2 (int)((2 / V_MUL) - 1)
#define ANI_INTERVAL_4 (int)((4 / V_MUL) - 1)
#define ANI_INTERVAL_8 (int)((8 / V_MUL) - 1)
#define ANI_INTERVAL_16 (int)((16 / V_MUL) - 1)
#define ANI_INTERVAL_32 (int)((32 / V_MUL) - 1)
#define ANI_INTERVAL_64 (int)((64 / V_MUL) - 1)
#define ANI_INTERVAL_128 (int)((128 / V_MUL) - 1)
#define ANI_INTERVAL_256 (int)((256 / V_MUL) - 1)

// UI defines

#define UI_MARGIN_NONE 0
#define UI_MARGIN_TIGHT 1
#define UI_MARGIN_SAFE 2
#define UI_MARGIN_WIDEZOOM 4

// 0 is no margin.
// 1 is 8px (1 tile) on top and bottom (totalling 16px), or roughly 93% NTSC
// 2 gives canonical 85% NTSC title safe
// 4 gives GBA screen height if used on top margin. Fits crop zooming TVs.
#define UI_MARGIN_TOP UI_MARGIN_TIGHT

#define UI_MARGIN_LEFT 1 // Don't change UI_MARGIN_LEFT to any other than 1

#define UI_MSGBOX_HEIGHT 6 // not including text. Text height is -2
#define UI_MSGBOX_ML_START (27 - UI_MSGBOX_HEIGHT - UI_MARGIN_TOP)
#define UI_MSGBOX_SL_START (27 - UI_MARGIN_TOP - 1)
#define UI_MAPSCREEN_SL_START (27 - UI_MARGIN_TOP)

// Due to the way the compiler works
// make sure that they're in the order from highest frequency to lowest
// for every switch case

#define ROUTINE_GAMELOOP 0
#define ROUTINE_GAMELOOP_RELOAD 1
#define ROUTINE_PAUSE 2

#define ROUTINE_MSGBOX 10

#define ROUTINE_MAPDISPLAY 100
#define ROUTINE_MAPDISPLAY_INIT 101

#define ROUTINE_FADEIN 65500
#define ROUTINE_FADEOUT 65501

#define ROUTINE_INIT 65535

#define ROUTINE_RESET 65534

#define DMA_QUEUE_MAX_ENTRIES 32 // entries per queue
#define DMA_QUEUE_MAX_LENGTH 4468 // 
//#define DMA_QUEUE_OVERHEAD 64 // time, in bytes lost to overhead, to loop a DMA queue entry in NMI
#define DMA_QUEUE_OVERHEAD 62 // time, in bytes lost to overhead, to loop a DMA queue entry in NMI

// Distance defines (squared)
// Note that they are in pixel distance squared.
// Must directly specify these numbers as otherwise odd issues happen
#define DIST_TILE_1 8 * 8
#define DIST_TILE_2 16 * 16
#define DIST_TILE_4 32 * 32
#define DIST_TILE_8 64 * 64
#define DIST_TILE_16 128l * 128l
#define DIST_TILE_32 256l * 256l
#define DIST_TILE_40 320l * 320l
#define DIST_TILE_48 384l * 384l

#define DIST_MELEE DIST_TILE_2
#define DIST_NORMAL DIST_TILE_4
#define DIST_TARGET_RANGE DIST_TILE_8
#define DIST_AI_MAX DIST_TILE_40

#define OBJ_MAX_COUNT 64
#define HIT_MAX_COUNT 32

#define STATE_IDLE 0
#define STATE_SWITCH_OFF 0
#define STATE_SWITCH_ON 1
#define STATE_MOVE_WALK 10
#define STATE_MOVE_RUN 11
#define STATE_ATTACK_BASIC 20
#define STATE_ATTACK_BASIC_MOVE 21
#define STATE_ATTACK_SPECIAL 30
#define STATE_ATTACK_SPECIAL_MOVE 31
#define STATE_HURT_NORMAL 40
#define STATE_HURT_NORMAL_MOVE 41
#define STATE_HURT_NORMAL_MOVE_RUN 42
#define STATE_HURT_BURN 50
#define STATE_HURT_BURN_MOVE 51
#define STATE_ICON_NORMAL 65500
#define STATE_ICON_BLINK 65501
#define STATE_ICON_HURT 65502
#define STATE_ICON_SPECIAL 65503
#define STATE_DIE 64000
#define STATE_SPAWNING 65000

#define FACING_UP 0
#define FACING_DOWN 1
#define FACING_LEFT 2
#define FACING_RIGHT 3
#define FACING_UPLEFT 4
#define FACING_UPRIGHT 5
#define FACING_DOWNLEFT 6
#define FACING_DOWNRIGHT 7
#define FACING_KEEPSAME 8

#define STATUS_NORMAL 0
#define STATUS_BURNING 1

#define AI_STATE_IDLE 0
#define AI_STATE_MOVE_TOWARDS 1
#define AI_STATE_MOVE_AWAY 2
#define AI_STATE_ATTACK 3

#define DMG_MULTIPLIER 1

#define PLAYER_HEALTH_STARTING 50

#define PLAYER_ATTACK_INTERVAL_NORMAL ((4) / V_MUL)
#define PLAYER_ATTACK_INTERVAL_SPECIAL ((4) / V_MUL)
#define PLAYER_ATTACK_TTL ((60) / V_MUL)
#define PLAYER_ATTACK_VALUE 1
#define PLAYER_ATTACK_MULT_MELEE 10
#define PLAYER_ATTACK_MULT_RANGED 5

#define PLAYER_DEFENSE_VALUE 1

#define ENEMY_HEALTH_STARTING 15

#define ENEMY_ATTACK_TTL ((120) / V_MUL)
#define ENEMY_ATTACK_VALUE 1
#define ENEMY_ATTACK_MULT_MELEE 16
#define ENEMY_ATTACK_MULT_RANGED 8

#define ENEMY_DEFENSE_VALUE 1

#define ENEMY_DROP_MONEY_MIN 100
#define ENEMY_DROP_MONEY_MAX 200
#define ENEMY_DROP_REC_AMOUNT 5

#define FX_SMOKE_INTERVAL ANI_INTERVAL_16 // Must be a power of 2
#define FX_SMOKE_TTL ((60) / V_MUL)

#define TILEDATA_ADDR_GAME_MAP 0x0000
#define TILEDATA_ADDR_GAME_UI_4BPP 0x5000
#define TILEDATA_ADDR_GAME_UI_2BPP 0x4000
#define TILEDATA_ADDR_SPRITES 0x6000

#define TILEDATA_ADDR_MAP_UI 0x4800

#define TILEMAP_ADDR_GAME_UI_4BPP 0x3000
#define TILEMAP_ADDR_GAME_UI_2BPP 0x3400
#define TILEMAP_ADDR_GAME_MAP 0x3800
#define TILEMAP_ADDR_MAP_MAP 0x4800
#define TILEMAP_ADDR_MAP_UI 0x4c00

#define LZ4_BUFFER_ADDR 0x007f0000

#define BLENDMODE_ADDSUB 0
#define BLENDMODE_ALPHA_TOWARDS_BLACK 1

/*
    HDMA channel setup:
    0: RESERVED for Vblank DMAs
    1: Palette (CGADD+CGDATA)
    2: Palette (CGADD+CGDATA)

    6: Message box sprite enable/disable (TM)
    7: RESERVED for Vblank Odd Frame DMAs
*/
#define HDMA_USED_CHANNELS_NORMAL 0x02
#define HDMA_USED_CHANNELS_MSGBOX 0x46
//#define HDMA_USED_CHANNELS 0xfe // All channels

#define PAL_UI_TEXT_WHITE 0
#define PAL_UI_BORDER 1
#define PAL_UI_TEXT_ERROR 2
#define PAL_UI_TEXT_BLACK 3
#define PAL_UI_4BPP 0

#define TM_MODE1 0x17 // BG1, BG2, BG3, and OBJ
#define TM_MODE1_MSGBOX 0x07 // BG1, BG2, BG3, and OBJ
#define TM_MODE3 0x13 // BG1, BG2, and OBJ

#define LEVEL_INITIAL &data_level_test_0