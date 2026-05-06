#include <snes/console.h>
#include <stdint.h>

#include "vars.h"

#include "dma.h"
#include "interrupt.h"
#include "interrupt_sub.h"

#if VBCC_ASM == 1
    NO_INLINE void interrupt_vblank_sub()
#else
    void interrupt_vblank_sub()
#endif
{
    // Write the current INIDISP value
    REG_INIDISP = shadow_inidisp;

    // Get the STAT77 value and set if an overflow happened
    shadow_stat77 = REG_STAT77;

    // Check the current routine and set up HDMA
    if (system_current_routine == ROUTINE_MSGBOX)
    {
        REG_HDMAEN = HDMA_USED_CHANNELS_MSGBOX;
    }
    else if (system_current_routine == ROUTINE_MAPDISPLAY || system_current_routine == ROUTINE_FADEIN || system_current_routine == ROUTINE_FADEOUT)
    {
        ; // Do not interfere with HDMA
    }
    else 
    {
        REG_HDMAEN = HDMA_USED_CHANNELS_NORMAL;
    }

    system_nmis_counted++;
    
    if (system_in_vblank && ((system_nmis_counted >= NMI_WAIT_COUNT) || (system_current_routine == ROUTINE_FADEIN) || (system_current_routine == ROUTINE_FADEOUT)))
    {
        // Game has finished processing
        system_in_vblank = 0; // Clear the vblank flag now to prevent problems later.
        system_nmis_counted = 0;

        dma_copy_oam();
        dma_copy_palette();

        dma_queue_process();

        // Write background values
        bg_scroll_y_mod.full.high.a = bg_scroll_y.full.high.a - 1;

        if (system_ui_in_bg2 == 0)
        {
            REG_BG2HOFS = bg_scroll_x.full.high.lh.l;
            REG_BG2HOFS = bg_scroll_x.full.high.lh.h;
            REG_BG2VOFS = bg_scroll_y_mod.full.high.lh.l;
            REG_BG2VOFS = bg_scroll_y_mod.full.high.lh.h;
        }
        else
        {
            REG_BG1HOFS = bg_scroll_x.full.high.lh.l;
            REG_BG1HOFS = bg_scroll_x.full.high.lh.h;
            REG_BG1VOFS = bg_scroll_y_mod.full.high.lh.l;
            REG_BG1VOFS = bg_scroll_y_mod.full.high.lh.h;
        }

        // Write the current MOSAIC value
        REG_MOSAIC = shadow_mosaic;

        system_frames_elapsed++;
    }
    else if (system_current_routine != ROUTINE_MAPDISPLAY)
    {
        // Just refresh the entries used by HDMA
        //dma_copy_palette_subset(0x22, 4); // UI message box: not needed as part of the table resets it
        dma_copy_palette_subset(0x42, 13); // Water tiles

        if (ani_bg_water_dma_ready)
        {
            dma_copy_bg_water_anim();

            ani_bg_water_dma_ready = 0;
        }

        if (ani_bg_tallbg_dma_ready)
        {
            dma_copy_bg_64height_anim();

            ani_bg_tallbg_dma_ready = 0;
        }
    }

    return;
}

