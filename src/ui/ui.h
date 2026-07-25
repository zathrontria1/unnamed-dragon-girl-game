#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>

#include "snes/console.h"
#include "consts.h"

struct game_object;

#define MENUACTION_NO 0
#define MENUACTION_YES 1

#define MENUACTION_OPTION_0 0
#define MENUACTION_OPTION_1 1
#define MENUACTION_OPTION_2 2
#define MENUACTION_OPTION_3 3

#define MENUACTION_OPENSUBSCREEN 128
#define MENUACTION_EXITSUBSCREEN 129
#define MENUACTION_CHANGEROUTINE 130
#define MENUACTION_OPENMAPSCREEN 131
#define MENUACTION_CALLFUNCTION 255

#define TRANSITION_STATE_NONE 0
#define TRANSITION_STATE_FADE_OUT 1
#define TRANSITION_STATE_INITIALIZE 2
#define TRANSITION_STATE_FADE_IN 3

extern ZP bool ui_in_subscreen;
extern bool ui_in_bg2;

// UI cache invalidation stuff
extern bool ui_force_update;
extern int32_t ui_cached_hp;
extern int32_t ui_cached_hp_max;
extern uint32_t ui_cached_money;
extern uint16_t ui_cached_enemy_counter;

// TODO: Handle UI windows and texts generically
extern uint16_t ui_window_background[(SCREEN_HEIGHT >> 3)][32]; // BG1. Call functions to draw a window here.
extern uint16_t ui_window_text[(SCREEN_HEIGHT >> 3)][32]; // BG3. Call functions to draw text here.

extern uint8_t ui_show_message_string[31]; // 30 characters + null terminator

// Sub-strings
extern uint16_t ui_hp_gauge[28];
extern uint16_t ui_money_counter[11];
extern uint16_t ui_enemy_counter[9];

extern uint16_t ui_level_status[5];

extern uint32_t ui_display_money;

// UI status and timers
extern uint16_t ui_show_message_ttl;
extern bool ui_show_message_cleared;

void UserInterface_Process(void);
void UserInterface_UpdateHealthCounters(void);
void UserInterface_UpdateMoneyCounters(void);
void UserInterface_UpdateEnemyCounters(void);

void UserInterface_PrintText(char * string_ptr, uint16_t row, uint16_t col);
void UserInterface_PrintText_Mode3(char * string_ptr, uint16_t row, uint16_t col);

// Genericized window drawing functions
void UserInterface_ClearWindowBuffer(bool use_clear_tile);
void UserInterface_ClearTextBuffer();
void UserInterface_ClearTextBuffer_Line(uint16_t y);
void UserInterface_DrawWindowBackground(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void UserInterface_DrawWindowBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void UserInterface_DrawWindowText(char * string_ptr, uint16_t x, uint16_t y);

void UserInterface_CopyUiBuffers();
void UserInterface_CopyTextBuffer_Line(uint16_t y);

void UserInterface_DrawEnemyHealthBar(struct game_object * o);

void UserInterface_CopyUiGraphicsToVram(void);

void UserInterface_Internal_Format3U(char *buf, uint16_t val);

#endif /* UI_H */
