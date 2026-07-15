#include <snes/console.h>

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "map.h"
#include "ui.h"
#include "dma.h"
#include "hdma.h"
#include "interrupt.h"
#include "interrupt_sub.h"
#include "gfx.h"

#include "ani_bg.h"

#include "system.h"

#if VBCC_ASM == 1
    NO_INLINE void Nmi_Primary()
#else
    void Nmi_Primary()
#endif
{
    // Write the current INIDISP value
    REG_INIDISP = shadow_fblank_enable | (shadow_brightness >> 8);

    // Get the STAT77 value and set if an overflow happened
    shadow_stat77 = REG_STAT77;

    REG_HDMAEN = shadow_hdmaen;
    REG_A1T2LH = hdma_gradient_ptr; // Repoint the HDMA table

    system_nmis_counted++;
    
    if ((system_in_vblank && (system_nmis_counted >= NMI_WAIT_COUNT)) || (system_current_routine == ROUTINE_FADEIN))
    {
        // Game has finished processing
        system_in_vblank = false; // Clear the vblank flag now to prevent problems later.
        system_nmis_counted = 0;

        // Repoint the HDMA table
        REG_A1T3LH = hdma_scroll_ptr;
        REG_A1T4LH = hdma_coldata_ptr;
        
        DmaSystem_UploadOam();
        DmaSystem_UploadCgram();

        DmaSystem_ProcessQueue();

        // Write background values
        bg_scroll_y_mod.full.high.a = bg_scroll_y.full.high.a - 1;

        if (!ui_in_bg2)
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

        System_UpdateFrameCounters();
    }
    else 
    {
        // Just refresh the entries used by HDMA
        DmaSystem_UploadCgram_Subset(0x04, 4); // UI message box
        DmaSystem_UploadCgram_Subset(0x39, 6); // Water tiles

        if (!system_suppress_odd_transfers)
        {
            if (ani_bg_water_dma_ready)
            {
                DmaSystem_UpdateStripTiles();

                ani_bg_water_dma_ready = 0;
            }

            if (ani_bg_tallbg_dma_ready)
            {
                DmaSystem_UpdateFrameTiles();

                ani_bg_tallbg_dma_ready = 0;
            }
        }
        else
        {
            ani_bg_water_dma_ready = 0;
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
    NO_INLINE void Nmi_Alternate()
#else
    void Nmi_Alternate()
#endif
{
    // Write the current INIDISP value
    REG_INIDISP = shadow_fblank_enable | (shadow_brightness >> 8);

    // Always reset them if an alternate NMI is used
    system_in_vblank = false; // Clear the vblank flag now to prevent problems later.
    system_nmis_counted = 0;

    // Adjust brightness based on the change
    if ((shadow_brightness == 0x0000) && (shadow_brightness_change < 0))
    {
        shadow_brightness_change = 0;
    }

    if ((shadow_brightness >= 0x0f00) && (shadow_brightness_change > 0))
    {
        shadow_brightness_change = 0;
    }

    if (shadow_brightness_change != 0)
    {
        shadow_brightness += shadow_brightness_change;
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

/*
    For cutscene use, using fblank
*/
#if VBCC_ASM == 1
    NO_INLINE void Nmi_Cutscene()
#else
    void Nmi_Cutscene()
#endif
{
    // Enable fblank
    REG_INIDISP = 0x80 | (shadow_brightness >> 8);

    // Get the STAT77 value and set if an overflow happened
    shadow_stat77 = REG_STAT77;

    system_nmis_counted++;
    
    if (system_in_vblank)
    {
        // Game has finished processing
        system_in_vblank = false; // Clear the vblank flag now to prevent problems later.
        
        DmaSystem_UploadOam();
        DmaSystem_UploadCgram();

        DmaSystem_ProcessQueue();

        // Write background values

        // Write the current MOSAIC value
        REG_MOSAIC = shadow_mosaic;
        if (system_nmis_counted >= 2)
        {
            // A bit of a hack to ensure that this ticks at 30FPS, not 60
            System_UpdateFrameCounters();
            system_nmis_counted = 0;
        }
    }

    if ((system_nmis_counted >= NMI_WAIT_COUNT) && !system_dont_count_lag)
    {
        system_frames_lag++; // Increment on lag frame (more than 1 physical frame)
    }

    // Disable fblank
    REG_INIDISP = shadow_fblank_enable | (shadow_brightness >> 8);

    return;
}