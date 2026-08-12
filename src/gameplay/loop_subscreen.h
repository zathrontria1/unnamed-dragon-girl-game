#ifndef LOOP_SUBSCREEN_H
#define LOOP_SUBSCREEN_H

#include <stdint.h>
#include <stdbool.h>

#include "ui.h"

#define MENUACTION_OPENSUBSCREEN 128
#define MENUACTION_EXITSUBSCREEN 129
#define MENUACTION_OPENMAPSCREEN 131
#define MENUACTION_CALLFUNCTION 255

#define TRANSITION_STATE_NONE 0
#define TRANSITION_STATE_FADE_OUT 1
#define TRANSITION_STATE_INITIALIZE 2
#define TRANSITION_STATE_FADE_IN 3

#define SUBSCREEN_ACTIONSTATE_NONE 0
#define SUBSCREEN_ACTIONSTATE_RETURN 1
#define SUBSCREEN_ACTIONSTATE_EXIT 2

struct menu_item {
    int16_t x;
    int16_t y;
    uint16_t action;
    void * ptr;
    uint8_t padding[6];
};

extern uint16_t subscreen_selection;
extern uint16_t subscreen_selection_profile;
extern uint16_t subscreen_bottom_entry;

extern bool subscreen_is_in_profile;
extern bool subscreen_restore_sprite_page;

extern bool subscreen_rendered;
extern bool subscreen_skip_window_redraw;

extern uint8_t subscreen_cgadsub_copy;

extern uint16_t subscreen_transition_state;

extern const struct menu_item subscreen_items_toplevel[7];
extern const struct menu_item subscreen_items_profile[5];
extern const struct menu_item subscreen_items_help[7];
extern const struct menu_item subscreen_items_options[9];
extern const struct menu_item subscreen_items_resetconfirm[3];

void Subscreen_Top();
void Subscreen_Top_DrawTime();
void Subscreen_Top_DrawTime_Internal_Format_6Chars(char *dest, uint16_t val);
void Subscreen_Top_DrawTime_Internal_Format_2Chars_ZeroPadded(char *dest, uint16_t val);

void Subscreen_Upgrade();
void Subscreen_Internal_SaveLastSpritePage();
void Subscreen_Upgrade_UploadProfilePicture();
void Subscreen_Internal_RestoreLastSpritePage();
void Subscreen_Upgrade_CalculateUpgradeCosts();
void Subscreen_Upgrade_DrawText(bool copy_result);
void Subscreen_Upgrade_Hp();
void Subscreen_Upgrade_Attack();
void Subscreen_Upgrade_Defense();

void Subscreen_Help();
void Subscreen_Help_DrawText(bool copy_result);

void Subscreen_Options();
void Subscreen_Options_DrawTile(uint16_t x, uint16_t y, uint16_t tile);
void Subscreen_Options_DrawValues(bool copy_result);

void Subscreen_ResetConfirmation();

void Subscreen_Internal_InitState();
void Subscreen_Internal_FindBottomEntry(const struct menu_item * item_array);
uint16_t Subscreen_Internal_HandleMenuAction(const struct menu_item * item_array);
void Subscreen_Internal_GoBackToTop();

void Subscreen_Internal_UpdateNavigation(const struct menu_item * item_array);

void Subscreen_Transition_Start(void * next_func);
void Loop_Subscreen_Transition_Init();
void Loop_Subscreen_Transition_FadeIn();
void Subscreen_Transition_Exit();

#endif /* LOOP_SUBSCREEN_H */

