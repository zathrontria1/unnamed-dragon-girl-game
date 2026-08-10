#ifndef HDMA_H
#define HDMA_H

#include <stdint.h>
#include <stdbool.h>
#include "consts.h"
#include "defs_structs.h"
#include "snes/console.h"

#define CACHE_PALETTE_ENTRIES 8

extern const struct hdma_indirect_table_entry hdma_bgpalette_tables[3];
extern uint16_t hdma_bgpalette_data[SCREEN_HEIGHT << 1];

extern const struct hdma_indirect_table_entry hdma_windowbackground_tables[2][4];
extern uint16_t hdma_windowbackground_data[2][SCREEN_HEIGHT << 1];

extern uint8_t hdma_use_gradient;
extern uint16_t hdma_gradient_ptr;

extern const struct hdma_indirect_table_entry hdma_scroll_tables[2][8];
extern uint16_t hdma_scroll_data[2][32];

extern uint16_t hdma_scroll_select;
extern uint16_t hdma_scroll_ptr;
extern uint16_t hdma_scroll_sine_index;

extern bool hdma_coldata_usegradient;
extern uint16_t hdma_coldata_last_r;
extern uint16_t hdma_coldata_last_g;
extern uint16_t hdma_coldata_last_b;
extern struct hdma_indirect_table_entry hdma_coldata_tables[2][193];
extern uint16_t hdma_coldata_data[2][32][4];

extern uint16_t hdma_coldata_select;
extern uint16_t hdma_coldata_ptr;

extern const uint8_t const_hdma_tm_msgbox[];
extern const int16_t const_hdma_scroll_sine[16][64];

extern const uint8_t const_scaled_color_table[32][64];

void HdmaEngine_SetupHdma();

void HdmaEngine_SetupPaletteHdma();
void HdmaEngine_SetupBgScrollHdma();
void HdmaEngine_SetupColdataHdma();
void HdmaEngine_UpdateBgScrollValues();
#if VBCC_ASM == 1
NO_INLINE void HdmaEngine_UpdateColdataValues();
#else
void HdmaEngine_UpdateColdataValues();
#endif


void HdmaEngine_GeneratePaletteTable(uint16_t * table_ptr, uint16_t pal_start, uint16_t entries, uint16_t target_color, uint16_t alpha, uint16_t height);

void HdmaEngine_EnableHdma();

void HdmaEngine_SetHdmaShadow();

#endif // HDMA_H


