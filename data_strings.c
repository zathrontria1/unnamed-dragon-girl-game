#include <stdint.h>
#include <stdbool.h>

#include "data_strings.h"

const uint8_t STR_MSG_TEST_SINGLELINE[] = "The quick brown fox jumps over";
const uint8_t STR_MSG_TEST_MULTILINE[] = "\
The quick brown fox jumps over\n\
the lazy dog.`1234567890@#$%^&\n\
ABCDEFGHIJKLMNOPQRSTUVWXYZ*()_\n\
Final line! +{}|:\"<>?-=[]\\;',/";

const uint8_t STR_MSG_TEST_MULTIPAGE[] = "\
First page of a multi-page\n\
text box.\r\
Second page of a multi-page\n\
text box.\r\
Third page of a multi-page\n\
text box.\r\
Final page of a multi-page\n\
text box.";

const uint8_t STR_MSG_TUTORIAL_MP[] = "\
Welcome to\n\
Unnamed Dragon Girl Game!\n\
A Button: Start Game\n\
X Button: Open Menu\r\
This is a prototype game,\n\
and I hope you have fun\n\
trying the game out.\n\
                 - Zathrontria";

const uint8_t STR_MSG_INCOMBAT[] = "Can't use while in combat";
const uint8_t STR_MSG_FOUNDMONEY[] = "Found %u money!";

const uint8_t STR_UI_HELP_MAP[] = "Press any button to close.";

const uint8_t STR_UI_SUBSCREEN_RESUME[] =  "Resume";
const uint8_t STR_UI_SUBSCREEN_PROFILE[] = "Profile";
const uint8_t STR_UI_SUBSCREEN_MAP[] =     "Map";
const uint8_t STR_UI_SUBSCREEN_HELP[] =    "Help";
const uint8_t STR_UI_SUBSCREEN_OPTIONS[] = "Options";
const uint8_t STR_UI_SUBSCREEN_RESTART[] = "Restart";

const uint8_t STR_UI_SUBSCREEN_MONEY[] =            "Money:       %10u";
const uint8_t STR_UI_SUBSCREEN_PLAYTIME[] =         "Play time: %6u:%02u:%02u";
const uint8_t STR_UI_SUBSCREEN_PLAYTIME_NOCOLON[] = "Play time: %6u %02u %02u";
const uint8_t STR_UI_SUBSCREEN_LAGCOUNTER[] =       "Lag counter: %10u";

const uint8_t STR_UI_SUBSCREEN_PROFILE_HEADING[] = "Profile";

const uint8_t STR_UI_SUBSCREEN_PROFILE_HEALTH[] =  "HP  %u";
const uint8_t STR_UI_SUBSCREEN_PROFILE_HEALTH_DIV[] =  "/%u";
const uint8_t STR_UI_SUBSCREEN_PROFILE_ATTACK[] =  "ATK %10u";
const uint8_t STR_UI_SUBSCREEN_PROFILE_DEFENSE[] = "DEF %10u";

const uint8_t STR_UI_SUBSCREEN_PROFILE_MONEY[] = "You have %u money."; 
const uint8_t STR_UI_SUBSCREEN_PROFILE_UPGRADE_HP[] = "Upgrade HP +10";
const uint8_t STR_UI_SUBSCREEN_PROFILE_UPGRADE_ATTACK[] = "Upgrade ATK +1";
const uint8_t STR_UI_SUBSCREEN_PROFILE_UPGRADE_DEFENSE[] = "Upgrade DEF +1";
const uint8_t STR_UI_SUBSCREEN_PROFILE_COST[] = "Cost: %u";

const uint8_t STR_UI_SUBSCREEN_PROFILE_UPGRADE_SUCCESS[] = "Upgrade complete!";
const uint8_t STR_UI_SUBSCREEN_PROFILE_UPGRADE_FAILURE[] = "Not enough money.";

const uint8_t STR_UI_SUBSCREEN_PROFILE_RETURN[] = "Back";

const uint8_t STR_UI_SUBSCREEN_HELP_HEADING[] = "Help";

const uint8_t STR_UI_SUBSCREEN_HELP_MOVEMENT_H[] = "1";
const uint8_t STR_UI_SUBSCREEN_HELP_INTERACTION_H[] = "2";
const uint8_t STR_UI_SUBSCREEN_HELP_ATTACK_H[] = "3";
const uint8_t STR_UI_SUBSCREEN_HELP_PROGRESSION_H[] = "4";
const uint8_t STR_UI_SUBSCREEN_HELP_MAP_H[] = "5";
const uint8_t STR_UI_SUBSCREEN_HELP_RESET_H[] = "6";

const uint8_t STR_UI_SUBSCREEN_HELP_MOVEMENT[] = "\
Press the D-Pad to move in\n\
that direction.\n\
You can move diagonally.\n\
\n\
Your facing will be in the\n\
same direction as what\n\
you moved towards.\n\
\n\
While moving, hold the\n\
B Button to run.\n\
Note that you can't run\n\
while breathing fire.";

const uint8_t STR_UI_SUBSCREEN_HELP_INTERACTION[] = "\
Press the A Button\n\
to read signs, talk to\n\
NPCs, operate switches,\n\
levers, and do stuff.\n\
\n\
To pick up items,\n\
walk over them.\n\
\n\
You can't interact with\n\
items that require the\n\
use of the A Button\n\
while you're in combat.";

const uint8_t STR_UI_SUBSCREEN_HELP_ATTACK[] = "\
Hold down the Y Button to\n\
breath fire in a cone.\n\
This does damage \n\
against a wide area.\n\
\n\
Press the A Button during\n\
combat to punch.\n\
The range is very limited,\n\
but it deals much more\n\
damage, and enemies have\n\
less invulnerability time.";

const uint8_t STR_UI_SUBSCREEN_HELP_PROGRESSION[] = "\
Defeat most enemies in a\n\
level, then find the entry\n\
to the next level, to \n\
progress in the game.\n\
\n\
If your HP drops to zero,\n\
it's Game Over.\n\
\n\
You'll regen HP if\n\
you have less than half\n\
remaining, but you'll need\n\
recovery items to recover\n\
HP past that.";

const uint8_t STR_UI_SUBSCREEN_HELP_MAP[] = "\
Select \"Map\" in Pause Menu\n\
to open the overview map\n\
for the current level.\n\
\n\
Press any button\n\
to resume the game.";

const uint8_t STR_UI_SUBSCREEN_HELP_RESET[] = "\
To restart the game,\n\
either: \n\
\n\
- Select \"Restart\"\n\
  in the Pause Menu\n\
\n\
- Press the following\n\
  combination while\n\
  the game is not paused:\n\
\n\
  L + R + SELECT + START\n\
\n\
Unsaved progress will\n\
be lost.";

const uint8_t STR_UI_SUBSCREEN_RESETCONFIRMATION[] = "\
Unsaved progress will be lost.\n\
Restart the game?";

const uint8_t STR_UI_SUBSCREEN_CONFIRM_YES[] = "Yes";
const uint8_t STR_UI_SUBSCREEN_CONFIRM_NO[] = "No";

const uint8_t STR_LEVELNAME_DEBUG_B1F[] = "DEBUG Dungeon B1F";
const uint8_t STR_LEVELNAME_DEBUG_B2F[] = "DEBUG Dungeon B2F";
const uint8_t STR_LEVELNAME_DEBUG_B3F[] = "DEBUG Dungeon B3F";
