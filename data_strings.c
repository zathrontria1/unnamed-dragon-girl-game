#include <stdint.h>
#include "data_strings.h"

uint8_t STR_MSG_TEST_SINGLELINE[] = "The quick brown fox jumps over";
uint8_t STR_MSG_TEST_MULTILINE[] = "\
The quick brown fox jumps over\n\
the lazy dog.`1234567890@#$%^&\n\
ABCDEFGHIJKLMNOPQRSTUVWXYZ*()_\n\
Final line! +{}|:\"<>?-=[]\\;',/";

uint8_t STR_MSG_TEST_MULTIPAGE[] = "\
First page of a multi-page\n\
text box.\r\
Second page of a multi-page\n\
text box.\r\
Third page of a multi-page\n\
text box.\r\
Final page of a multi-page\n\
text box.";

uint8_t STR_MSG_TUTORIAL_MP[] = "\
Welcome to\n\
Unnamed Dragon Girl Game!\n\
Press the A Button to read\n\
the instructions.\r\
Use the D-Pad to move around.\n\
You can move faster if you\n\
hold down the B Button.\r\
The A Button is used to\n\
interact with the world.\n\
This is also the button\n\
to punch things.\r\
Hold down the Y Button\n\
to unleash flames.\n\
Lower damage, and slows you,\n\
but has good coverage.\r\
Press the X Button\n\
to check your stats.\r\
Press the START Button\n\
to pause the game.\n\
Press the SELECT Button\n\
to view the dungeon map.\r\
This is a prototype game,\n\
and I hope you have fun\n\
trying the game out.\n\
                 - Zathrontria";

uint8_t STR_MSG_INCOMBAT[] = "Can't use while in combat";

uint8_t STR_UI_HELP_MAP[] = "SELECT Button: Return to game";

uint8_t STR_UI_PLAYERINFO_ML[] = "\
HP: %u/%u\n\
ATK: %u DEF: %u\n\
Money: %u \n\
Play time:        %6u:%02u:%02u";

uint8_t STR_LEVELNAME_DEBUG_B1F[] = "DEBUG Dungeon B1F";
uint8_t STR_LEVELNAME_DEBUG_B2F[] = "DEBUG Dungeon B2F";
uint8_t STR_LEVELNAME_DEBUG_B3F[] = "DEBUG Dungeon B3F";
