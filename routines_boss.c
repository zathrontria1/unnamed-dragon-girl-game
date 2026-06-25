#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "spr.h"
#include "spr_metaspr.h"

#include "ani.h"

#include "obj.h"
#include "dma.h"

#include "routines_boss.h"

#include "math_int.h"

#include "ui.h"

#include "snd.h"
#include "consts_snd.h"

#include "hittest.h"

#include "gfx.h"

#include "movement.h"

#include "main.h"

uint16_t obj_boss_state; // Boss state machine variable

void Routines_Boss_Test(struct game_object * o)
{
    if (!system_game_paused)
    {
        struct game_object * p = obj_player_pointer;

        if ((o->state != STATE_DIE) && (o->state != STATE_SPAWNING))
        {
            // Place boss logic here
            event_in_combat = 1;
        }
        else if (o->state == STATE_DIE)
        {
            // Place events to take place when the boss dies here
            ;
        }
        else // spawning
        {
            // Delay when a boss is spawning
            event_in_combat = 1;
            
            o->struct_data.npc_data.status_time--;
            if (o->struct_data.npc_data.status_time == 0)
            {
                o->state = STATE_IDLE;
            }
        }
    }
    

    return;
}
