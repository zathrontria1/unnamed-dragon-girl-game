#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "ui.h"
#include "dma.h"
#include "hdma.h"
#include "interrupt.h"
#include "interrupt_sub.h"
#include "gfx.h"

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
    if (hdma_use_gradient == 0xffff)
    {
        REG_HDMAEN = HDMA_USED_CHANNELS_MSGBOX;
        // Repoint the HDMA table
        REG_A1T2LH = hdma_gradient_ptr;
        
    }
    else if (hdma_use_gradient == 0)
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

        // Repoint the HDMA table
        REG_A1T3LH = hdma_scroll_ptr;
        
        dma_copy_oam();
        dma_copy_palette();

        dma_queue_process();

        // Write background values
        bg_scroll_y_mod.full.high.a = bg_scroll_y.full.high.a - 1;

        if (ui_in_bg2 == 0)
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

        // Write the current colour math values
        REG_CGWSEL = shadow_cgwsub;
        REG_CGADSUB = shadow_cgadsub;
        REG_COLDATA = shadow_coldata_r;
        REG_COLDATA = shadow_coldata_g;
        REG_COLDATA = shadow_coldata_b;

        system_frames_elapsed++;
    }
    else if (system_current_routine != ROUTINE_MAPDISPLAY)
    {
        // Just refresh the entries used by HDMA
        dma_copy_palette_subset(0x04, 4); // UI message box
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

        if ((system_nmis_counted >= NMI_WAIT_COUNT) && !system_dont_count_lag)
        {
            system_frames_lag++; // Increment on lag frame (more than 2 physical frames)
        }
    }

    return;
}

/*
    This special NMI routine is used to only perform the fader for visual consistency
    during heavy processing elsewhere
*/
#if VBCC_ASM == 1
    NO_INLINE void interrupt_vblank_alt()
#else
    void interrupt_vblank_alt()
#endif
{
    // Write the current INIDISP value
    REG_INIDISP = shadow_inidisp;

    // Always reset them if an alternate NMI is used
    system_in_vblank = 0; // Clear the vblank flag now to prevent problems later.
    system_nmis_counted = 0;

    // Adjust the INIDISP value based on the change
    if ((shadow_inidisp == 0x00) && (shadow_inidisp_change < 0))
    {
        shadow_inidisp_change = 0;
    }

    if ((shadow_inidisp >= 0x0f) && (shadow_inidisp_change > 0))
    {
        shadow_inidisp_change = 0;
    }

    if (shadow_inidisp_change != 0)
    {
        shadow_inidisp += shadow_inidisp_change;
    }

    // Do not interfere with HDMA

    // Reuse the existing mosaic code, plenty of time
    Gfx_ProcessMosaic();

    // Write the current MOSAIC value
    REG_MOSAIC = shadow_mosaic;

    // Write the current colour math values
    REG_CGWSEL = shadow_cgwsub;
    REG_CGADSUB = shadow_cgadsub;
    REG_COLDATA = shadow_coldata_r;
    REG_COLDATA = shadow_coldata_g;
    REG_COLDATA = shadow_coldata_b;

    // Do not update the frame counter

    return;
}