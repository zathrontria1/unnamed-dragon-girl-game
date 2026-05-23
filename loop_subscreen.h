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

struct menu_item {
    int16_t x;
    int16_t y;
    uint16_t action;
    void * ptr;
    uint8_t padding[6];
};

extern bool subscreen_rendered;
extern uint16_t subscreen_selection;
extern uint16_t subscreen_bottom_entry;

extern uint16_t subscreen_cursor_x;
extern uint16_t subscreen_cursor_y;

extern const struct menu_item subscreen_items_toplevel[7];
extern const struct menu_item subscreen_items_profile[5];
extern const struct menu_item subscreen_items_help[7];
extern const struct menu_item subscreen_items_resetconfirm[3];

void loop_subscreen_top();
void loop_subscreen_top_drawtime();

void loop_subscreen_profile();
void loop_subscreen_profile_drawtext();

void loop_subscreen_help();
void loop_subscreen_help_drawtext(bool copy_result);

void loop_subscreen_resetconfirm();
