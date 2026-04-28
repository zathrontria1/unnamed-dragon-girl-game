#include <snes/console.h>
#include <stdint.h>

#include "vars.h"

#include "map.h"
#include "dma.h"

#include "math_int.h"

/*
    Sets the map and metatile LUT pointers,
    then loads the tilemap data.
*/
void map_load(const uint8_t * map, const uint16_t * lut, const uint8_t * col)
{
    map_current = map;
    map_extent_tiles_x = (*map) << 4;
    map_extent_x = (*map++) << 8;
    map_extent_tiles_y = (*map) << 4;
    map_extent_y = (*map) << 8;

    map_lut = lut;
    map_lut_col = col;

    // Ensure that the camera is at a valid position
    map_camera_adjust(1);

    map_regenerate();

    return;
}

void map_regenerate()
{
    const uint8_t * p = (map_current+2);

    int16_t temp_x_tile_offset = (bg_scroll_x.full.high.a >> 4); 
    int16_t temp_y_tile_offset = (bg_scroll_y.full.high.a >> 4) - 1; 
    int16_t temp_x_tile_offset_adj = temp_x_tile_offset - 7;
    int16_t temp_x_tile_offset_adj_target = temp_x_tile_offset_adj + 32;

    uint16_t temp_odd = 0;

    // Unpack the tilemap one column at a time. Will need to be repeated 32 times
    // provide the tile index (in metatile terms, so a screen is 16x16)
    // or put it in another way
    // bg scroll div 16 on each axis
    // note that for the purposes of the tilemap builder always start slightly from the left and top edges - 7 and 1 tiles off on X and Y.
    for (int16_t i = temp_x_tile_offset_adj - 1; i < temp_x_tile_offset_adj_target; )
    {
        uint16_t temp_section = map_tilemap_build_col(p, map_lut, i, temp_y_tile_offset, temp_odd);

        uint16_t temp_x_wrap = (((temp_x_tile_offset+24) & 0x1f) << 1) & 0x1f;

        if (temp_odd == 0x0000)
        {
            dma_queue_add((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section << 10)+temp_x_wrap), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0);
            temp_odd = 0x0001;
        }
        else
        {
            dma_queue_add((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section << 10)+temp_x_wrap+1), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0);
            temp_odd = 0x0000;
            temp_x_tile_offset++;
            i++;
        }

        dma_queue_process();
    }

    return;
}

/*
    Adjust the camera based on the player position.

    Map tile generation relies on this being correct.

    If the player is not initialized yet, all background scrolls are assumed to be 0.

    Unless otherwise specified, map column or row generation will take place when
    the camera crosses a metatile boundary. This should be suppressed only during 
    initial map load.
*/
void map_camera_adjust(uint16_t suppress_map_gen)
{
    struct game_object * ptr = &objects[obj_player_index];

    if (bg_scroll_use_interpolation == 0)
    {
        bg_scroll_x.full.high.a = (*ptr).pos.x.lh.h - 120;
        bg_scroll_y.full.high.a = (*ptr).pos.y.lh.h - 104;
    }
    else
    {
        int16_t temp_x_camadjust;
        int16_t temp_y_camadjust;

        if (bg_scroll_x_bounds_min.full.high.a != -32768)
        {
            if (
                (((*ptr).pos.x.lh.h - 120) > bg_scroll_x_bounds_min.full.high.a) &&
                (((*ptr).pos.x.lh.h - 120) < bg_scroll_x_bounds_max.full.high.a))
            {
                temp_x_camadjust = (*ptr).pos.x.lh.h - 120;
            }
            else
            {
                if (((*ptr).pos.x.lh.h - 120) < bg_scroll_x_bounds_min.full.high.a)
                {
                    temp_x_camadjust = bg_scroll_x_bounds_min.full.high.a;
                }
                else
                {
                    temp_x_camadjust = bg_scroll_x_bounds_max.full.high.a;
                }
            }

            if (
                (((*ptr).pos.y.lh.h - 104) > bg_scroll_y_bounds_min.full.high.a) &&
                (((*ptr).pos.y.lh.h - 104) < bg_scroll_y_bounds_max.full.high.a))
            {
                temp_y_camadjust = (*ptr).pos.y.lh.h - 104;
            }
            else
            {
                if (((*ptr).pos.y.lh.h - 104) < bg_scroll_y_bounds_min.full.high.a)
                {
                    temp_y_camadjust = bg_scroll_y_bounds_min.full.high.a;
                }
                else
                {
                    temp_y_camadjust = bg_scroll_y_bounds_max.full.high.a;
                }
            }
        }
        else
        {
            temp_x_camadjust = (*ptr).pos.x.lh.h - 120;
            temp_y_camadjust = (*ptr).pos.y.lh.h - 104;
        }

        if (temp_x_camadjust < 0)
        {
            temp_x_camadjust = 0;
        }
        else if (temp_x_camadjust > map_extent_x - 256)
        {
            temp_x_camadjust = map_extent_x - 256;
        }

        if (temp_y_camadjust < 0)
        {
            temp_y_camadjust = 0;
        }
        else if (temp_y_camadjust > map_extent_y - 224)
        {
            temp_y_camadjust = map_extent_y - 224;
        }

        // Get the angle between the intended scroll position and the current scroll position.
        // The values if no adjustment were done like normal cases
        int16_t unadjusted_bg_scroll_x = (*ptr).pos.x.lh.h - 120;
        int16_t unadjusted_bg_scroll_y = (*ptr).pos.y.lh.h - 104;

        int16_t temp_x;
        int16_t temp_y;

        if (bg_scroll_x_bounds_min.full.high.a != -32768)
        {
            temp_x = temp_x_camadjust - unadjusted_bg_scroll_x;
            temp_y = temp_y_camadjust - unadjusted_bg_scroll_y;
        }
        else
        {
            temp_x = unadjusted_bg_scroll_x - bg_scroll_x.full.high.a;
            temp_y = unadjusted_bg_scroll_y - bg_scroll_y.full.high.a;
        }

        uint8_t angle = (uint8_t)((uint8_t)(atan2_uint8(temp_x, temp_y)) + (uint8_t)(128));

        // Apply the angle change again
        int32_t temp_delta_x = data_sine_1[angle] * (2 * V_MUL);
        int32_t temp_delta_y = data_cosine_1[angle] * (2 * V_MUL);

        bg_scroll_x.a += temp_delta_x;
        bg_scroll_y.a += temp_delta_y;

        // Check if exceed target values and snap them to the pixel grid
        // Depends on if a screen lock area is set or not.
        if (bg_scroll_x_bounds_min.full.high.a != -32768)
        {
            bg_scroll_x_at_final = 0;
            bg_scroll_y_at_final = 0;

            if (data_sine_1[angle] >= 0)
            {
                if (bg_scroll_x.full.high.a >= (temp_x_camadjust - (2 * V_MUL)))
                {
                    bg_scroll_x.full.high.a = temp_x_camadjust;
                    bg_scroll_x.full.sub = 0;

                    bg_scroll_x_at_final = 1;
                }
            }
            else
            {
                if (bg_scroll_x.full.high.a <= (temp_x_camadjust + (2 * V_MUL)))
                {
                    bg_scroll_x.full.high.a = temp_x_camadjust;
                    bg_scroll_x.full.sub = 0;

                    bg_scroll_x_at_final = 1;
                }
            }

            if (data_cosine_1[angle] >= 0)
            {
                if (bg_scroll_y.full.high.a >= (temp_y_camadjust - (2 * V_MUL)))
                {
                    bg_scroll_y.full.high.a = temp_y_camadjust;
                    bg_scroll_y.full.sub = 0;

                    bg_scroll_y_at_final = 1;
                }
            }
            else
            {
                if (bg_scroll_y.full.high.a <= (temp_y_camadjust + (2 * V_MUL)))
                {
                    bg_scroll_y.full.high.a = temp_y_camadjust;
                    bg_scroll_y.full.sub = 0;

                    bg_scroll_y_at_final = 1;
                }
            }

            if ((bg_scroll_x_at_final + bg_scroll_y_at_final) >= 2)
            {
                bg_scroll_use_interpolation = 0;
            }
        }
        else
        {
            bg_scroll_x_at_final = 0;
            bg_scroll_y_at_final = 0;

            if (data_sine_1[angle] >= 0)
            {
                if (bg_scroll_x.full.high.a >= (temp_x_camadjust - (2 * V_MUL)))
                {
                    bg_scroll_x.full.high.a = temp_x_camadjust;
                    bg_scroll_x.full.sub = 0;

                    bg_scroll_x_at_final = 1;
                }
            }
            else
            {
                if (bg_scroll_x.full.high.a <= (temp_x_camadjust + (2 * V_MUL)))
                {
                    bg_scroll_x.full.high.a = temp_x_camadjust;
                    bg_scroll_x.full.sub = 0;

                    bg_scroll_x_at_final = 1;
                }
            }

            if (data_cosine_1[angle] >= 0)
            {
                if (bg_scroll_y.full.high.a >= (temp_y_camadjust - (2 * V_MUL)))
                {
                    bg_scroll_y.full.high.a = temp_y_camadjust;
                    bg_scroll_y.full.sub = 0;

                    bg_scroll_y_at_final = 1;
                }
            }
            else
            {
                if (bg_scroll_y.full.high.a <= (temp_y_camadjust + (2 * V_MUL)))
                {
                    bg_scroll_y.full.high.a = temp_y_camadjust;
                    bg_scroll_y.full.sub = 0;

                    bg_scroll_y_at_final = 1;
                }
            }

            if ((bg_scroll_x_at_final + bg_scroll_y_at_final) >= 2)
            {
                bg_scroll_use_interpolation = 0;
            }
        }
    }

    // Limit the bounds
    // Combat extent bounds check
    if ((bg_scroll_x_bounds_min.full.high.a != -32768) && (bg_scroll_use_interpolation == 0))
    {
        if (bg_scroll_x.full.high.a > bg_scroll_x_bounds_max.full.high.a)
        {
            bg_scroll_x.full.high.a = bg_scroll_x_bounds_max.full.high.a;
            bg_scroll_x.full.sub = 0;
        }
        else if (bg_scroll_x.full.high.a < bg_scroll_x_bounds_min.full.high.a)
        {
            bg_scroll_x.full.high.a = bg_scroll_x_bounds_min.full.high.a;
            bg_scroll_x.full.sub = 0;
        }

        if (bg_scroll_y.full.high.a > bg_scroll_y_bounds_max.full.high.a)
        {
            bg_scroll_y.full.high.a = bg_scroll_y_bounds_max.full.high.a;
            bg_scroll_y.full.sub = 0;
        }
        else if (bg_scroll_y.full.high.a < bg_scroll_y_bounds_min.full.high.a)
        {
            bg_scroll_y.full.high.a = bg_scroll_y_bounds_min.full.high.a;
            bg_scroll_y.full.sub = 0;
        }
    }

    // Map extent bounds check
    if (bg_scroll_x.full.high.a < 0)
    {
        bg_scroll_x.full.high.a = 0;
    }
    else if (bg_scroll_x.full.high.a > (map_extent_x - 256))
    {
        bg_scroll_x.full.high.a = (map_extent_x - 256);
    }

    if (bg_scroll_y.full.high.a < 0)
    {
        bg_scroll_y.full.high.a = 0;
    }
    else if (bg_scroll_y.full.high.a > (map_extent_y - 224))
    {
        bg_scroll_y.full.high.a = (map_extent_y - 224);
    }

    if (suppress_map_gen == 0)
    {
        map_check_tilemap_crossing();

        // previous values are already updated within function
        return;
    }

    bg_scroll_x_prev.full.high.a = bg_scroll_x.full.high.a;
    bg_scroll_y_prev.full.high.a = bg_scroll_y.full.high.a;

    return;
}

// Split into its own function to make code neater
void map_check_tilemap_crossing()
{
    const uint8_t * p = map_current;
    p += 2;

    uint16_t temp_x_odd = (bg_scroll_x.full.high.a >> 3) & 0x0001;
    int16_t temp_x_tile_offset_8 = bg_scroll_x.full.high.a >> 3;
    int16_t temp_x_tile_offset = bg_scroll_x.full.high.a >> 4;
    int16_t temp_x_tile_offset_prev_8 = bg_scroll_x_prev.full.high.a >> 3;
    int16_t temp_y_tile_offset = bg_scroll_y.full.high.a >> 4;

    if (temp_x_tile_offset_8 > temp_x_tile_offset_prev_8)
    {
        if ((temp_x_tile_offset+16) < map_extent_tiles_x)
        {
            uint16_t temp_section = map_tilemap_build_col(p, map_lut, temp_x_tile_offset+24, temp_y_tile_offset-1, temp_x_odd);

            uint16_t temp_x_wrap = (((temp_x_tile_offset+24) & 0x1f) << 1) & 0x1f;

            if (temp_x_odd == 0x0000)
            {
                dma_queue_add((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section * 1024)+temp_x_wrap), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0);
            }
            else
            {
                dma_queue_add((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section * 1024)+temp_x_wrap+1), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0);
            }
        }
    }
    else if (temp_x_tile_offset_8 < temp_x_tile_offset_prev_8)
    {
        if ((temp_x_tile_offset -7) >= -7)
        {
            uint16_t temp_section = map_tilemap_build_col(p, map_lut, temp_x_tile_offset-7, temp_y_tile_offset-1, temp_x_odd);

            uint16_t temp_x_wrap = (((temp_x_tile_offset+25) & 0x1f) << 1) & 0x1f;

            if (temp_x_odd == 0x0000)
            {
                dma_queue_add((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section * 1024)+temp_x_wrap), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0);
            }
            else
            {
                dma_queue_add((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section * 1024)+temp_x_wrap+1), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0);
            }
        }
    }

    bg_scroll_x_prev.full.high.a = bg_scroll_x.full.high.a;

    // Now to test the Y axis
    uint16_t temp_y_odd = (bg_scroll_y.full.high.a >> 3) & 0x0001;

    int16_t temp_y_tile_offset_8 = bg_scroll_y.full.high.a >> 3;
    temp_y_tile_offset = bg_scroll_y.full.high.a >> 4;
    int16_t temp_y_tile_offset_prev_8 = bg_scroll_y_prev.full.high.a >> 3;
    temp_x_tile_offset = bg_scroll_x.full.high.a >> 4;

    if (temp_y_tile_offset_8 > temp_y_tile_offset_prev_8)
    {
        map_tilemap_build_row(p, map_lut, temp_x_tile_offset - 7, temp_y_tile_offset + 14, temp_y_odd);

        uint16_t temp_y_wrap = ((((temp_y_tile_offset+14) & 0x0f) << 1) & 0x1f) << 5;

        // Sections are not important for rows as both maps will be updated.
        if (temp_y_odd == 0x0000)
        {
            dma_queue_add((uint8_t *)&map_row[0][0], (TILEMAP_ADDR_GAME_MAP+temp_y_wrap), 64, VRAM_INCHIGH, 0);
            dma_queue_add((uint8_t *)&map_row[1][0], (TILEMAP_ADDR_GAME_MAP+1024+temp_y_wrap), 64, VRAM_INCHIGH, 0);
        }
        else
        {
            dma_queue_add((uint8_t *)&map_row[0][0], (TILEMAP_ADDR_GAME_MAP+32+temp_y_wrap), 64, VRAM_INCHIGH, 0);
            dma_queue_add((uint8_t *)&map_row[1][0], (TILEMAP_ADDR_GAME_MAP+32+1024+temp_y_wrap), 64, VRAM_INCHIGH, 0);
        }
    }
    else if (temp_y_tile_offset_8 < temp_y_tile_offset_prev_8)
    {
        map_tilemap_build_row(p, map_lut, temp_x_tile_offset - 7, temp_y_tile_offset - 1, temp_y_odd);

        uint16_t temp_y_wrap = ((((temp_y_tile_offset+15) & 0x0f) << 1) & 0x1f) << 5;

        // Sections are not important for rows as both maps will be updated.
        if (temp_y_odd == 0x0000)
        {
            dma_queue_add((uint8_t *)&map_row[0][0], (TILEMAP_ADDR_GAME_MAP+temp_y_wrap), 64, VRAM_INCHIGH, 0);
            dma_queue_add((uint8_t *)&map_row[1][0], (TILEMAP_ADDR_GAME_MAP+1024+temp_y_wrap), 64, VRAM_INCHIGH, 0);
        }
        else
        {
            dma_queue_add((uint8_t *)&map_row[0][0], (TILEMAP_ADDR_GAME_MAP+32+temp_y_wrap), 64, (VRAM_INCHIGH), 0);
            dma_queue_add((uint8_t *)&map_row[1][0], (TILEMAP_ADDR_GAME_MAP+32+1024+temp_y_wrap), 64, (VRAM_INCHIGH), 0);
        }
    }

    bg_scroll_y_prev.full.high.a = bg_scroll_y.full.high.a;

    return;
}

/*
    Tilemap build a column during live game scrolling

    Can also be invoked to force a full refresh during fblank if needed
*/
uint16_t map_tilemap_build_col(const uint8_t * p, const uint16_t * lut, int16_t tile_x, int16_t tile_y, uint16_t odd)
{
    
    // tile_x to determine if it's on the odd or even section of the tilemap.
    // if x = 0:  0 + 16 = 16, 16 >> 4 = 1, 1-1 = 0, 0 & 0x01 = 0
    // if x = 17: 17+ 16 = 33, 33 >> 4 = 2, 2-1 = 1, 1 & 0x01 = 1
    uint16_t temp_section = ((((tile_x + 16) >> 4) - 1 )& 0x0001); // Offset it by 16 so that the values are always positive, then shift right by 4 and take just the lowest bit.
    
    // values here pertain to the buffer
    uint16_t j = ((tile_y + 16) & 0x0f); // the offset on Y axis
    uint16_t k = ((tile_x + 16) & 0x0f); // the offset on X axis

    uint16_t temp_screen_x;
    uint16_t temp_screen_y;

    uint16_t temp_start_x;

    if ((tile_x >= 0) && (tile_x < map_extent_tiles_x))
    {
        temp_start_x = (tile_x & 0xf);
        temp_screen_x = tile_x >> 4;
    }
    else
    {
        temp_start_x = 0;
        temp_screen_x = 0;
    }

    uint16_t temp_start_y;
    if ((tile_y >= 0) && (tile_y < map_extent_tiles_y))
    {
        temp_start_y = (tile_y & 0xf);
        temp_screen_y = tile_y >> 4;
    }
    else
    {
        temp_start_y = 0;
        temp_screen_y = 0;
    }

    uint16_t temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

    p += temp_screen_offset + temp_start_x + ((temp_start_y) << 4);

    for (int i = 0; i < 16; i++)
    {
        if ((tile_x < 0) || (tile_y < 0) || (tile_x >= map_extent_tiles_x) || (tile_y >= map_extent_tiles_y))
        {
            int16_t l = j << 1;
            // Out of range tile
            map_column[l] = 0;
            map_column[l+1] = 0;

            // Do not increment pointer
        }
        else
        {
            int16_t l = j << 1;
            int16_t q = (*p) << 2;
            if (odd == 0)
            {
                map_column[l] = lut[q];
                map_column[l+1] = lut[q+2];
            }
            else
            {
                map_column[l] = lut[q+1];
                map_column[l+1] = lut[q+3];
            }

            p += 16; // Next row in source data
        }

        tile_y++; // next row in absolute tile count
        if (((tile_y & 0x0f) == 0) && (tile_y > 0))
        {
            // A screen edge has been exceeded
            p += ((((map_extent_x >> 8) - 1) << 8));
        }

        j++; // Next row in Y offset within the tilemap
        if (j >= 16)
        {
            j = 0;
        }
    }

    return temp_section;
}

// ditto but for rows
void map_tilemap_build_row(const uint8_t * p, const uint16_t * lut, int16_t tile_x, int16_t tile_y, uint16_t odd)
{
    // tile_x to determine if it's on the odd or even section of the tilemap.
    
    uint16_t temp_section = ((((tile_x + 16) >> 4) - 1 )& 0x0001); // Offset it by 16 so that the values are always positive, then shift right by 4 and take just the lowest bit.
    
    // if -7, 15...
    uint16_t j = (tile_y + 16) & 0x0f; // the offset on Y axis
    uint16_t k = (tile_x + 32) & 0x1f; // the offset on X axis

    uint16_t temp_internal_section = k >> 4;
    
    uint16_t l = k & 0xf; // Use this for writing the tilemap entries, which are only realistically 16 wide

    uint16_t temp_screen_x;
    uint16_t temp_screen_y;

    uint16_t temp_start_x;
    if ((tile_x >= 0) && (tile_x < map_extent_tiles_x))
    {
        temp_start_x = tile_x & 0x0f;
        temp_screen_x = tile_x >> 4;
    }
    else
    {
        temp_start_x = 0;
        temp_screen_x = 0;
    }

    uint16_t temp_start_y;
    if ((tile_y >= 0) && (tile_y < map_extent_tiles_y))
    {
        temp_start_y = tile_y & 0x0f;
        temp_screen_y = tile_y >> 4;
    }
    else
    {
        temp_start_y = 0;
        temp_screen_y = 0;
    }

    uint16_t temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

    p += temp_screen_offset + temp_start_x + (temp_start_y << 4);

    for (int i = 0; i < 32; i++) // Two effective rows, L and R.
    {
        int16_t m = l << 1;
        if ((tile_x < 0) || (tile_y < 0) || (tile_x >= map_extent_tiles_x) || (tile_y >= map_extent_tiles_y))
        {
            
            // Out of range tile
            map_row[temp_internal_section][m] = 0;
            map_row[temp_internal_section][m+1] = 0;
            // Do not increment pointer
        }
        else
        {
            // Note: for row building:
            // [0][i] is for the left 2 rows
            // [1][i] is for the right 2 rows.
            // Change the map selection after crossing a boundary.
            if (odd == 0)
            {
                map_row[temp_internal_section][(l << 1)] = lut[((*p) << 2)]; // Top left
                map_row[temp_internal_section][(l << 1)+1] = lut[((*p) << 2) + 1]; // Top right
            }
            else
            {
                map_row[temp_internal_section][(l << 1)] = lut[((*p) << 2) + 2]; // Bottom left
                map_row[temp_internal_section][(l << 1)+1] = lut[((*p) << 2) + 3]; // Bottom right
            }
            p++; // Next column in source data
        }

        tile_x++; // next column in absolute tile count
        if (((tile_x & 0x0f) == 0) && (tile_x > 0))
        {
            // A screen edge has been exceeded
            p += 240;
        }

        l++; // Next row in X offset within the tilemap row builder
        if (l >= 16)
        {
            l = 0;
            temp_internal_section ^= 0x0001;
        }
        
    }

    return;
}

