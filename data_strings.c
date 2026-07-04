#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "data_strings.h"

HUGE const uint8_t STR_MSG_TEST_SINGLELINE[] = "The quick brown fox jumps over";
HUGE const uint8_t STR_MSG_TEST_MULTILINE[] = "\
The quick brown fox jumps over\n\
the lazy dog.`1234567890@#$%^&\n\
ABCDEFGHIJKLMNOPQRSTUVWXYZ*()_\n\
Final line! +{}|:\"<>?-=[]\\;',/";

HUGE const uint8_t STR_MSG_TEST_MULTIPAGE[] = "\
First page of a multi-page\n\
text box.\r\
Second page of a multi-page\n\
text box.\r\
Third page of a multi-page\n\
text box.\r\
Final page of a multi-page\n\
text box.";

HUGE const uint8_t STR_MSG_TUTORIAL_BOSS[] = "\
You're approaching a boss room.\n\
Once you enter one,\n\
there's no going back until\n\
the boss goes down!\r\
To proceed, hit the switch\n\
near the door.";

HUGE const uint8_t STR_MSG_TUTORIAL_MP[] = "\
Welcome to\n\
Unnamed Dragon Girl Game!\n\
A Button: Start Game\n\
X Button: Open Menu\r\
This is a prototype game,\n\
and I hope you have fun\n\
trying the game out.\n\
                 - Zathrontria";

HUGE const uint8_t STR_MSG_INCOMBAT[] = "Can't use while in combat";
HUGE const uint8_t STR_MSG_FOUNDMONEY[] = "Found %ju money!";

HUGE const uint8_t STR_UI_HELP_MAP[] = "Press any button to close.";

HUGE const uint8_t STR_UI_SUBSCREEN_RESUME[] =  "Resume";
HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE[] = "Profile";
HUGE const uint8_t STR_UI_SUBSCREEN_MAP[] =     "Map";
HUGE const uint8_t STR_UI_SUBSCREEN_HELP[] =    "Help";
HUGE const uint8_t STR_UI_SUBSCREEN_OPTIONS[] = "Options";
HUGE const uint8_t STR_UI_SUBSCREEN_RESTART[] = "Restart";

HUGE const uint8_t STR_UI_SUBSCREEN_MONEY[] =            "Money:       %10lu";
HUGE const uint8_t STR_UI_SUBSCREEN_PLAYTIME[] =         "Play time: %6u:%02u:%02u";
HUGE const uint8_t STR_UI_SUBSCREEN_PLAYTIME_NOCOLON[] = "Play time: %6u %02u %02u";
HUGE const uint8_t STR_UI_SUBSCREEN_LAGCOUNTER[] =       "Lag counter: %10lu";

HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_HEADING[] = "Profile";

HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_HEALTH[] =  "HP  %lu";
HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_HEALTH_DIV[] =  "/%lu";
HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_ATTACK[] =  "ATK %10u";
HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_DEFENSE[] = "DEF %10u";

HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_MONEY[] = "You have %lu money."; 
HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_UPGRADE_HP[] = "Upgrade HP +10";
HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_UPGRADE_ATTACK[] = "Upgrade ATK +1";
HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_UPGRADE_DEFENSE[] = "Upgrade DEF +1";
HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_COST[] = "Cost: %lu";

HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_UPGRADE_SUCCESS[] = "Upgrade complete!";
HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_UPGRADE_FAILURE[] = "Not enough money.";

HUGE const uint8_t STR_UI_SUBSCREEN_PROFILE_RETURN[] = "Back";

HUGE const uint8_t STR_UI_SUBSCREEN_HELP_HEADING[] = "Help";

HUGE const uint8_t STR_UI_SUBSCREEN_HELP_MOVEMENT_H[] = "1";
HUGE const uint8_t STR_UI_SUBSCREEN_HELP_INTERACTION_H[] = "2";
HUGE const uint8_t STR_UI_SUBSCREEN_HELP_ATTACK_H[] = "3";
HUGE const uint8_t STR_UI_SUBSCREEN_HELP_PROGRESSION_H[] = "4";
HUGE const uint8_t STR_UI_SUBSCREEN_HELP_MAP_H[] = "5";
HUGE const uint8_t STR_UI_SUBSCREEN_HELP_RESET_H[] = "6";

HUGE const uint8_t STR_UI_SUBSCREEN_HELP_MOVEMENT[] = "\
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

HUGE const uint8_t STR_UI_SUBSCREEN_HELP_INTERACTION[] = "\
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

HUGE const uint8_t STR_UI_SUBSCREEN_HELP_ATTACK[] = "\
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

HUGE const uint8_t STR_UI_SUBSCREEN_HELP_PROGRESSION[] = "\
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

HUGE const uint8_t STR_UI_SUBSCREEN_HELP_MAP[] = "\
Select \"Map\" in Pause Menu\n\
to open the overview map\n\
for the current level.\n\
\n\
Press any button\n\
to resume the game.";

HUGE const uint8_t STR_UI_SUBSCREEN_HELP_RESET[] = "\
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

HUGE const uint8_t STR_UI_SUBSCREEN_RESETCONFIRMATION[] = "\
Unsaved progress will be lost.\n\
Restart the game?";

HUGE const uint8_t STR_UI_SUBSCREEN_CONFIRM_YES[] = "Yes";
HUGE const uint8_t STR_UI_SUBSCREEN_CONFIRM_NO[] = "No";

HUGE const uint8_t STR_LEVELNAME_DEBUG_B1F[] = "DEBUG Dungeon B1F";
HUGE const uint8_t STR_LEVELNAME_DEBUG_B2F[] = "DEBUG Dungeon B2F";
HUGE const uint8_t STR_LEVELNAME_DEBUG_B3F[] = "DEBUG Dungeon B3F";

HUGE const uint8_t STR_ERROR_CONTROLLER[] = "\
\n\
\n\
\n\
\n\
\n\
\n\
\n\
An unsupported peripheral has been\n\
connected to a controller port.\n\
\n\
Turn off the console, remove any Mouse,\n\
Super Scope, or other unsupported peripheral,\n\
plug in a standard controller,\n\
then turn on the console again.\n\
\n\
If using an emulator, check your emulator\n\
documentation on how to configure input devices.";

HUGE const uint8_t STR_ERROR_REGION[] = "\
\n\
\n\
\n\
\n\
\n\
This console appears to be a 50Hz system.\n\
\n\
The game will play slower than intended, and\n\
streamed sampled audio playback may not\n\
function correctly and may pop.\n\
\n\
To play the game anyway, press the following\n\
button combination:\n\
\n\
L + R + START + SELECT\n\
\n\
or restart the console with the Reset button.\n\
\n\
If using an emulator, check your emulator\n\
documentation on how to change the console\n\
refresh rate/region.";

HUGE const uint8_t STR_STARTUP[] = "\
\n\
\n\
\n\
\n\
Zathrontria presents\n\
\n\
Unnamed Dragon Girl Game (temp.)\n\
\n\
WORK IN PROGRESS - NOT FINAL\n\
\n\
Find the latest build and source at:\n\
    https://github.com/zathrontria1/\n\
    unnamed-dragon-girl-game\n\
\n\
[NOTICE]\n\
This game saves automatically when\n\
you enter a new level or floor.\n\
\n\
Don't turn off the power\n\
while entering or exiting a staircase,\n\
or a door that does not have a clear\n\
destination.";

HUGE const uint8_t STR_TITLE_START[] = "PRESS START BUTTON";
HUGE const uint8_t STR_GAME_OVER[] = "GAME OVER";
