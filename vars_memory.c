#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "maps/map_lut_dungeon.h"
#include "maps/map_lut_dungeon_col.h"
#include "maps/map_debug0.h"
#include "maps/map_debug1.h"
#include "data_tables.h"
#include "data_tables_lut.h"
#include "data_metaspr_list.h"

#include "consts.h"
#include "consts_snd.h"
#include "sound/seq/seq_test.h"

// For SRAM management
const uint8_t const_sram_verify_str[] = "EIEIMUN!"; // Can use any 8 character string that isn't all 0x00 or 0xff. Will occupy 9 bytes in ROM
uint8_t sram_available_slots;

// For DMA/HDMA copies
const uint32_t const_zero = 0;
const uint8_t const_sprite_offscreen = 0xf0;

const uint8_t const_hdma_tm_msgbox[] = 
{
    UI_MSGBOX_ML_START * 4, TM_MODE1, 
    UI_MSGBOX_ML_START * 4, TM_MODE1, 
    UI_MSGBOX_HEIGHT * 8, TM_MODE1_MSGBOX, 
    1, TM_MODE1, 
    0, 
};

const uint16_t const_ui_textadvance_tilemapentries[] =
{
    0x0060 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
    0x0061 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
    0x0070 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
    0x0071 | 0x2000 | (PAL_UI_TEXT_WHITE << 10),
};

// System general shadows and variables
ZP uint16_t system_in_vblank;

ZP uint16_t system_current_routine;
ZP uint16_t system_target_routine;

ZP uint32_t system_frames_elapsed;
ZP uint32_t system_frames_lag;
uint16_t system_dont_count_lag; // set to 1 to not increment the lag counter during e.g. creating a text box

ZP uint16_t system_nmis_counted;
ZP uint16_t system_use_alternate_nmi;

ZP uint16_t system_game_paused;

ZP void * system_loop_func_ptr;

// Shadow buffers
ZP uint8_t shadow_inidisp;
ZP int8_t shadow_inidisp_change;

ZP uint8_t shadow_mosaic;

ZP uint8_t shadow_cgwsub;
ZP uint8_t shadow_cgadsub;
ZP uint8_t shadow_coldata_r;
ZP uint8_t shadow_coldata_g;
ZP uint8_t shadow_coldata_b;

ZP uint8_t shadow_stat77;

NEAR union oam_buffer shadow_oam;
NEAR union cgram_full shadow_cgram; // 256 palette entries, 2 bytes wide each

// Event flags
uint8_t event_flags_global[EVENT_FLAG_GLOBAL_MAX];
uint8_t event_flags_local[EVENT_FLAG_LOCAL_MAX];

int16_t event_interaction_x;
int16_t event_interaction_y;

uint16_t event_in_combat;
uint16_t event_in_combat_shadow;

uint16_t event_tutorial_shown;

// Fixed sprite tile anims
uint16_t ani_bg_frame_coin;
uint8_t * ani_bg_addr_coin;
