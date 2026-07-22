#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"

#include "obj.h"

#include "map.h"
#include "dma.h"

#include "math_int.h"

// Map decompression buffers.
uint16_t map_column[32]; // one contiguous column
uint16_t map_row[2][32]; // two contiguous rows: left and right rows (2x32)

const uint8_t * map_current;
uint16_t map_extent_x;
uint16_t map_extent_y;
uint16_t map_extent_tiles_x;
uint16_t map_extent_tiles_y;

uint16_t map_extent_tiles_x_shiftcount; // converted into amount of shifts. 16 being 4, 32 being 5, 64 being 6, 128 being 7

const uint16_t * map_lut;
const uint8_t * map_lut_col;

// Collision buffer decompresses here for speed and editability
uint8_t map_collision_buf[64*64]; // 4KB // There is no speed benefit from making this 16-bit wide

// Camera/background scroll
ZP union pos_bgscroll bg_scroll_x;
ZP union pos_bgscroll bg_scroll_y;
union pos_bgscroll bg_scroll_x_prev;
union pos_bgscroll bg_scroll_y_prev;
ZP union pos_bgscroll bg_scroll_y_mod;

union pos_bgscroll bg_scroll_x_saved;
union pos_bgscroll bg_scroll_y_saved;

union pos_bgscroll bg_scroll_x_bounds_min;
union pos_bgscroll bg_scroll_y_bounds_min;
union pos_bgscroll bg_scroll_x_bounds_max;
union pos_bgscroll bg_scroll_y_bounds_max;

bool bg_scroll_use_interpolation;
bool bg_scroll_x_at_final;
bool bg_scroll_y_at_final;
bool bg_scroll_suppress_interpolation_state_change;

static uint8_t map_tilemap_recovery_state;
bool map_tilemap_recovery_pending;

/*
    Sets the map and metatile LUT pointers,
    then loads the tilemap data.
*/
/**
 * @brief Loads map grid bytes, dimensions, visual metatile LUTs, and collision tables.
 * 
 * @param map Pointer to the map grid array.
 * @param lut Pointer to the visual metatile lookup table.
 * @param col Pointer to the collision metatile lookup table.
 */
void MapSystem_LoadMap(const uint8_t * map, const uint16_t * lut, const uint8_t * col)
{
    map_current = map;
    map_extent_tiles_x = ((uint16_t)(*map)) << 4;
    map_extent_x = ((uint16_t)(*map++)) << 8;
    
    map_extent_tiles_y = ((uint16_t)(*map)) << 4;
    map_extent_y = ((uint16_t)(*map)) << 8;

    map_lut = lut;
    map_lut_col = col;

    map_extent_tiles_x_shiftcount = 0;

    uint16_t temp = map_extent_tiles_x >> 1;
    while (temp != 0)
    {
        temp >>= 1;
        map_extent_tiles_x_shiftcount++;
    }

    // Generate the collision map
    MapSystem_BuildCollisionTable();

    // Ensure that the camera is at a valid position
    MapSystem_UpdateCameraPosition(1);

    return;
}

/*
    Call to build the collision table
    The table will be completely linear, so only ends when it's out of tiles.
*/
/**
 * @brief Decompresses map collision metatiles into the WRAM collision matrix `map_collision_buf`.
 */
void MapSystem_BuildCollisionTable()
{
    const uint8_t * ptr = map_current;
    ptr += 2; 

    uint16_t temp_len = map_extent_tiles_x * map_extent_tiles_y;
    uint16_t x = 0;
    uint16_t y = 0;

    // Need to convert the map from screen-based to linear for further performance improvements
    for (unsigned int i = 0; i < temp_len; i++)
    {
        uint16_t temp_start_x = (x & 0xf);
        uint16_t temp_screen_x = x >> 4;

        uint16_t temp_start_y = (y & 0xf) << 4;
        uint16_t temp_screen_y = y >> 4;

        uint16_t temp_screen_offset = (temp_screen_x << 8) + (temp_screen_y << (6 + (map_extent_x >> 8)));

        //uint16_t q;
        const uint8_t * q;
        q = ptr + temp_screen_offset + temp_start_x + temp_start_y;

        map_collision_buf[i] = map_lut_col[*q];

        x++;
        if (x >= map_extent_tiles_x)
        {
            x = 0;
            y++;
        }

        //map_collision_buf[i] = map_lut_col[*ptr];
        //ptr++;
    }

    return;
}

/**
 * @brief Checks whether a given bounding box position overlaps a solid map tile or map boundary.
 * 
 * @param x Origin X coordinate.
 * @param y Origin Y coordinate.
 * @param w Width of bounding box.
 * @param h Height of bounding box.
 * @return true if position overlaps solid tile or boundary, false if clear.
 */
bool MapSystem_IsPositionSolid(int16_t x, int16_t y, int16_t w, int16_t h)
{
    if (w <= 0)
    {
        w = 16;
    }
    if (h <= 0)
    {
        h = 16;
    }

    if (bg_scroll_x_bounds_min.full.high.a != -32768)
    {
        int16_t min_x = bg_scroll_x_bounds_min.full.high.a;
        int16_t max_x = bg_scroll_x_bounds_max.full.high.a + 256;
        int16_t min_y = bg_scroll_y_bounds_min.full.high.a;
        int16_t max_y = bg_scroll_y_bounds_max.full.high.a + 224;

        if ((x < min_x) || (x + w > max_x) || (y < min_y) || (y + h > max_y))
        {
            return true;
        }
    }

    int16_t tx1 = (x + 1) >> 4;
    int16_t tx2 = (x + w - 2) >> 4;
    int16_t ty1 = (y + 1) >> 4;
    int16_t ty2 = (y + h - 2) >> 4;

    if (tx2 < tx1)
    {
        tx2 = tx1;
    }
    if (ty2 < ty1)
    {
        ty2 = ty1;
    }

    if ((tx1 < 0) || (tx2 >= (int16_t)map_extent_tiles_x) || (ty1 < 0) || (ty2 >= (int16_t)map_extent_tiles_y))
    {
        return true;
    }

    uint16_t shiftcount = map_extent_tiles_x_shiftcount;

    for (int16_t ty = ty1; ty <= ty2; ty++)
    {
        uint16_t shift_y = (uint16_t)ty << shiftcount;
        for (int16_t tx = tx1; tx <= tx2; tx++)
        {
            uint16_t idx = shift_y + (uint16_t)tx;
            if (map_collision_buf[idx] < MAP_COLL_ALLOW_MOVE)
            {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Evaluates map collision for overhead background tiles across an object's width and height, returning the sprite priority mask.
 * 
 * @param o                   Pointer to the game object.
 * @param width               Width of the metasprite in pixels.
 * @param height_check_offset Height offset (negative) from base Y to check for overhead structures.
 * @return uint16_t           0x2000 (Priority 2, behind) or 0x3000 (Priority 3, in front).
 */
uint16_t MapSystem_GetMetaspritePriority(struct game_object * o, int16_t width, int16_t height_check_offset)
{
    int16_t bx = o->pos.x.lh.h;
    int16_t by = o->pos.y.lh.h; // Ground Y position (shadow Y)
    
    int16_t top_y = by + height_check_offset;
    if (top_y < 0)
    {
        top_y = 0;
    }
    
    uint16_t start_tile_y = (uint16_t)top_y >> 4;
    uint16_t end_tile_y = (uint16_t)by >> 4;
    
    int16_t max_offset = (width > 16) ? (width - 16) : 0;
    int16_t offset_x;
    
    for (offset_x = 0; offset_x <= max_offset; offset_x += 16)
    {
        int16_t test_x = bx + offset_x;
        if (test_x >= 0)
        {
            uint16_t tile_x = (uint16_t)test_x >> 4;
            uint16_t ty;
            
            for (ty = start_tile_y; ty <= end_tile_y && ty < 64; ty++)
            {
                uint16_t idx = ((ty & 63) << 6) | (tile_x & 63);
                
                if (map_collision_buf[idx] & MAP_COLL_OVERHEAD)
                {
                    uint16_t scan_y = ty;
                    while (scan_y < 64)
                    {
                        uint16_t scan_idx = ((scan_y & 63) << 6) | (tile_x & 63);
                        if (!(map_collision_buf[scan_idx] & MAP_COLL_OVERHEAD))
                        {
                            break;
                        }
                        scan_y++;
                    }
                    
                    uint16_t structure_base_y = scan_y << 4;
                    
                    if (by < (int16_t)structure_base_y)
                    {
                        return 0x2000;
                    }
                }
            }
        }
    }

    return 0x3000;
}

/**
 * @brief Rebuilds the entire VRAM tilemap buffer.
 */
void MapSystem_Tilemap_RegenerateTilemap()
{
    const uint8_t * p = (map_current+2);

    int16_t temp_x_tile_offset = ((uint16_t)bg_scroll_x.full.high.a >> 4); 
    int16_t temp_y_tile_offset = ((uint16_t)bg_scroll_y.full.high.a >> 4) - 1; 
    int16_t temp_x_tile_offset_adj = temp_x_tile_offset - 7;
    int16_t temp_x_tile_offset_adj_target = temp_x_tile_offset_adj + 32;

    uint16_t temp_odd = 0;

    // Unpack the tilemap one column at a time. Will need to be repeated 32 times
    // provide the tile index (in metatile terms, so a screen is 16x16)
    // or put it in another way
    // bg scroll div 16 on each axis
    // note that for the purposes of the tilemap builder always start slightly from the left and top edges - 7 and 1 tiles off on X and Y.
    for (int i = temp_x_tile_offset_adj; i < temp_x_tile_offset_adj_target; )
    {
        uint16_t temp_section = MapSystem_Tilemap_BuildColumn(p, map_lut, i, temp_y_tile_offset, temp_odd);

        uint16_t temp_x_wrap = ((uint16_t)i & 0x000f) << 1;

        if (temp_odd == 0x0000)
        {
            DmaSystem_AddItemToQueue((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section << 10)+temp_x_wrap), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0);
            temp_odd = 0x0001;
        }
        else
        {
            DmaSystem_AddItemToQueue((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section << 10)+temp_x_wrap+1), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0);
            temp_odd = 0x0000;
            i++;
        }

        DmaSystem_ProcessQueue();
    }

    return;
}

void MapSystem_Tilemap_RequestEmergencyRecovery()
{
    map_tilemap_recovery_pending = true;

    return;
}

void MapSystem_Tilemap_StartEmergencyRecovery()
{
    if (!map_tilemap_recovery_pending || map_tilemap_recovery_state != 0)
    {
        return;
    }

    map_tilemap_recovery_pending = false;
    map_tilemap_recovery_state = 1;
    system_game_paused = true;
    system_dont_count_lag = true;
    system_use_alternate_nmi = true;
    shadow_brightness_change = 128 * V_MUL;
    system_loop_func_ptr = Main_GetFunctionPointer(ROUTINE_MAP_RECOVERY);

    return;
}

void MapSystem_Tilemap_EmergencyRecovery()
{
    if (map_tilemap_recovery_state == 1)
    {
        if (shadow_brightness < (15 << 8))
        {
            return;
        }

        REG_INIDISP = DSP_FORCEVBL | 0x0f;
        shadow_fblank_enable = DSP_FORCEVBL;
        DmaSystem_ResetQueue();
        MapSystem_Tilemap_RegenerateTilemap();

        map_tilemap_recovery_state = 2;
        shadow_brightness_change = (128 * V_MUL);
        shadow_fblank_enable = 0;
    }
    else if (map_tilemap_recovery_state == 2 && shadow_brightness <= 0)
    {
        map_tilemap_recovery_state = 0;
        system_game_paused = false;
        system_dont_count_lag = false;
        system_use_alternate_nmi = false;
        system_loop_func_ptr = Main_GetFunctionPointer(ROUTINE_GAMELOOP);
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
/**
 * @brief Updates camera scroll coordinates tracking the player position.
 * 
 * @param suppress_map_gen If true, skips queuing new VRAM row/column tile updates.
 */
void MapSystem_UpdateCameraPosition(bool suppress_map_gen)
{
    int16_t max_cam_x = map_extent_x - 256;
    int16_t max_cam_y = map_extent_y - 224;

    if (!bg_scroll_use_interpolation)
    {
        int16_t px = obj_player_pointer->pos.x.lh.h - 120;
        int16_t py = obj_player_pointer->pos.y.lh.h - 104;

        int16_t bounds_min_x = bg_scroll_x_bounds_min.full.high.a;
        if (bounds_min_x != -32768)
        {
            int16_t bounds_max_x = bg_scroll_x_bounds_max.full.high.a;
            if (px > bounds_max_x)
            {
                px = bounds_max_x;
                bg_scroll_x.full.sub = 0;
            }
            else if (px < bounds_min_x)
            {
                px = bounds_min_x;
                bg_scroll_x.full.sub = 0;
            }

            int16_t bounds_min_y = bg_scroll_y_bounds_min.full.high.a;
            int16_t bounds_max_y = bg_scroll_y_bounds_max.full.high.a;
            if (py > bounds_max_y)
            {
                py = bounds_max_y;
                bg_scroll_y.full.sub = 0;
            }
            else if (py < bounds_min_y)
            {
                py = bounds_min_y;
                bg_scroll_y.full.sub = 0;
            }
        }
        bg_scroll_x.full.high.a = px;
        bg_scroll_y.full.high.a = py;
    }
    else
    {
        int16_t temp_x_camadjust;
        int16_t temp_y_camadjust;

        int16_t target_x = obj_player_pointer->pos.x.lh.h - 120;
        int16_t target_y = obj_player_pointer->pos.y.lh.h - 104;

        int16_t bounds_min_x = bg_scroll_x_bounds_min.full.high.a;
        if (bounds_min_x != -32768)
        {
            int16_t bounds_max_x = bg_scroll_x_bounds_max.full.high.a;
            if (target_x < bounds_min_x)
            {
                temp_x_camadjust = bounds_min_x;
            }
            else if (target_x > bounds_max_x)
            {
                temp_x_camadjust = bounds_max_x;
            }
            else
            {
                temp_x_camadjust = target_x;
            }

            int16_t bounds_min_y = bg_scroll_y_bounds_min.full.high.a;
            int16_t bounds_max_y = bg_scroll_y_bounds_max.full.high.a;
            if (target_y < bounds_min_y)
            {
                temp_y_camadjust = bounds_min_y;
            }
            else if (target_y > bounds_max_y)
            {
                temp_y_camadjust = bounds_max_y;
            }
            else
            {
                temp_y_camadjust = target_y;
            }
        }
        else
        {
            temp_x_camadjust = target_x;
            temp_y_camadjust = target_y;
        }

        if (temp_x_camadjust < 0)
        {
            temp_x_camadjust = 0;
        }
        else if (temp_x_camadjust > max_cam_x)
        {
            temp_x_camadjust = max_cam_x;
        }

        if (temp_y_camadjust < 0)
        {
            temp_y_camadjust = 0;
        }
        else if (temp_y_camadjust > max_cam_y)
        {
            temp_y_camadjust = max_cam_y;
        }

        // Get the angle between the intended scroll position and the current scroll position.
        int16_t temp_x = temp_x_camadjust - bg_scroll_x.full.high.a;
        int16_t temp_y = temp_y_camadjust - bg_scroll_y.full.high.a;

        uint8_t angle = (uint8_t)((uint8_t)(Math_GetAtan2_u8(temp_x, temp_y)) + (uint8_t)(128));

        // Apply the angle change again
        int32_t temp_delta_x = Math_Sin(angle) * (2 * V_MUL);
        int32_t temp_delta_y = Math_Cos(angle) * (2 * V_MUL);

        bg_scroll_x.a += temp_delta_x;
        bg_scroll_y.a += temp_delta_y;

        // Check if exceed target values and snap them to the pixel grid
        bg_scroll_x_at_final = false;
        bg_scroll_y_at_final = false;

        int16_t dist_x = bg_scroll_x.full.high.a - temp_x_camadjust;
        if (dist_x < 0)
        {
            dist_x = -dist_x;
        }
        if (dist_x <= (2 * V_MUL))
        {
            bg_scroll_x.full.high.a = temp_x_camadjust;
            bg_scroll_x.full.sub = 0;
            bg_scroll_x_at_final = true;
        }

        int16_t dist_y = bg_scroll_y.full.high.a - temp_y_camadjust;
        if (dist_y < 0)
        {
            dist_y = -dist_y;
        }
        if (dist_y <= (2 * V_MUL))
        {
            bg_scroll_y.full.high.a = temp_y_camadjust;
            bg_scroll_y.full.sub = 0;
            bg_scroll_y_at_final = true;
        }

        if ((bg_scroll_x_at_final + bg_scroll_y_at_final) >= 2) // Intentional
        {
            bg_scroll_use_interpolation = false;
        }
    }

    // Map extent bounds check
    if (bg_scroll_x.full.high.a < 0)
    {
        bg_scroll_x.full.high.a = 0;
    }
    else if (bg_scroll_x.full.high.a > max_cam_x)
    {
        bg_scroll_x.full.high.a = max_cam_x;
    }

    if (bg_scroll_y.full.high.a < 0)
    {
        bg_scroll_y.full.high.a = 0;
    }
    else if (bg_scroll_y.full.high.a > max_cam_y)
    {
        bg_scroll_y.full.high.a = max_cam_y;
    }

    if (!suppress_map_gen)
    {
        MapSystem_CheckCrossedTilemapEdge();

        // previous values are already updated within function
        return;
    }

    bg_scroll_x_prev.full.high.a = bg_scroll_x.full.high.a;
    bg_scroll_y_prev.full.high.a = bg_scroll_y.full.high.a;

    return;
}

// Split into its own function to make code neater
/**
 * @brief Checks if camera movement crossed a 16px tile boundary and queues row/column updates.
 */
void MapSystem_CheckCrossedTilemapEdge()
{
    if (bg_scroll_x.full.high.a == bg_scroll_x_prev.full.high.a &&
        bg_scroll_y.full.high.a == bg_scroll_y_prev.full.high.a)
    {
        return;
    }

    int16_t temp_x_tile_offset_8 = (uint16_t)bg_scroll_x.full.high.a >> 3;
    int16_t temp_x_tile_offset_prev_8 = (uint16_t)bg_scroll_x_prev.full.high.a >> 3;
    int16_t temp_x_tile_offset_delta_8 = temp_x_tile_offset_8 - temp_x_tile_offset_prev_8;

    if ((temp_x_tile_offset_delta_8 > 1) || (temp_x_tile_offset_delta_8 < -1))
    {
        MapSystem_Tilemap_RequestEmergencyRecovery();
        bg_scroll_x_prev.full.high.a = bg_scroll_x.full.high.a;
        bg_scroll_y_prev.full.high.a = bg_scroll_y.full.high.a;
        return;
    }

    if (temp_x_tile_offset_delta_8 != 0)
    {
        const uint8_t * p = map_current + 2;
        bool temp_x_odd = temp_x_tile_offset_8 & 0x0001;
        int16_t temp_x_tile_offset = (uint16_t)bg_scroll_x.full.high.a >> 4;
        int16_t temp_y_tile_offset = (uint16_t)bg_scroll_y.full.high.a >> 4;

        if (temp_x_tile_offset_8 > temp_x_tile_offset_prev_8)
        {
            if ((temp_x_tile_offset+16) < map_extent_tiles_x)
            {
                uint16_t temp_section = MapSystem_Tilemap_BuildColumn(p, map_lut, temp_x_tile_offset+24, temp_y_tile_offset-1, temp_x_odd);

                uint16_t temp_x_wrap = (((temp_x_tile_offset+24) & 0x1f) << 1) & 0x1f;

                if (!temp_x_odd)
                {
                    if (DmaSystem_AddItemToQueue((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section * 1024)+temp_x_wrap), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0))
                    {
                        MapSystem_Tilemap_RequestEmergencyRecovery();
                    }
                }
                else
                {
                    if (DmaSystem_AddItemToQueue((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section * 1024)+temp_x_wrap+1), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0))
                    {
                        MapSystem_Tilemap_RequestEmergencyRecovery();
                    }
                }
            }
        }
        else
        {
            if ((temp_x_tile_offset -7) >= -7)
            {
                uint16_t temp_section = MapSystem_Tilemap_BuildColumn(p, map_lut, temp_x_tile_offset-7, temp_y_tile_offset-1, temp_x_odd);

                uint16_t temp_x_wrap = (((temp_x_tile_offset+25) & 0x1f) << 1) & 0x1f;

                if (!temp_x_odd)
                {
                    if (DmaSystem_AddItemToQueue((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section * 1024)+temp_x_wrap), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0))
                    {
                        MapSystem_Tilemap_RequestEmergencyRecovery();
                    }
                }
                else
                {
                    if (DmaSystem_AddItemToQueue((uint8_t *)&map_column[0], (TILEMAP_ADDR_GAME_MAP+(temp_section * 1024)+temp_x_wrap+1), 64, (VRAM_INCHIGH|VRAM_ADRSTINC_32), 0))
                    {
                        MapSystem_Tilemap_RequestEmergencyRecovery();
                    }
                }
            }
        }
    }

    bg_scroll_x_prev.full.high.a = bg_scroll_x.full.high.a;

    // Now to test the Y axis
    int16_t temp_y_tile_offset_8 = (uint16_t)bg_scroll_y.full.high.a >> 3;
    int16_t temp_y_tile_offset_prev_8 = (uint16_t)bg_scroll_y_prev.full.high.a >> 3;
    int16_t temp_y_tile_offset_delta_8 = temp_y_tile_offset_8 - temp_y_tile_offset_prev_8;

    if (temp_y_tile_offset_delta_8 > 1 || temp_y_tile_offset_delta_8 < -1)
    {
        MapSystem_Tilemap_RequestEmergencyRecovery();
        bg_scroll_y_prev.full.high.a = bg_scroll_y.full.high.a;
        return;
    }

    if (temp_y_tile_offset_delta_8 != 0)
    {
        const uint8_t * p = map_current + 2;
        bool temp_y_odd = temp_y_tile_offset_8 & 0x0001;
        int16_t temp_y_tile_offset = (uint16_t)bg_scroll_y.full.high.a >> 4;
        int16_t temp_x_tile_offset = (uint16_t)bg_scroll_x.full.high.a >> 4;

        if (temp_y_tile_offset_8 > temp_y_tile_offset_prev_8)
        {
            MapSystem_Tilemap_BuildRow(p, map_lut, temp_x_tile_offset - 7, temp_y_tile_offset + 14, temp_y_odd);

            uint16_t temp_y_wrap = ((((temp_y_tile_offset+14) & 0x0f) << 1) & 0x1f) << 5;

            // Sections are not important for rows as both maps will be updated.
            if (!temp_y_odd)
            {
                if (DmaSystem_AddItemToQueue((uint8_t *)&map_row[0][0], (TILEMAP_ADDR_GAME_MAP+temp_y_wrap), 64, VRAM_INCHIGH, 0))
                {
                    MapSystem_Tilemap_RequestEmergencyRecovery();
                }
                if (DmaSystem_AddItemToQueue((uint8_t *)&map_row[1][0], (TILEMAP_ADDR_GAME_MAP+1024+temp_y_wrap), 64, VRAM_INCHIGH, 0))
                {
                    MapSystem_Tilemap_RequestEmergencyRecovery();
                }
            }
            else
            {
                if (DmaSystem_AddItemToQueue((uint8_t *)&map_row[0][0], (TILEMAP_ADDR_GAME_MAP+32+temp_y_wrap), 64, VRAM_INCHIGH, 0))
                {
                    MapSystem_Tilemap_RequestEmergencyRecovery();
                }
                if (DmaSystem_AddItemToQueue((uint8_t *)&map_row[1][0], (TILEMAP_ADDR_GAME_MAP+32+1024+temp_y_wrap), 64, VRAM_INCHIGH, 0))
                {
                    MapSystem_Tilemap_RequestEmergencyRecovery();
                }
            }
        }
        else
        {
            MapSystem_Tilemap_BuildRow(p, map_lut, temp_x_tile_offset - 7, temp_y_tile_offset - 1, temp_y_odd);

            uint16_t temp_y_wrap = ((((temp_y_tile_offset+15) & 0x0f) << 1) & 0x1f) << 5;

            // Sections are not important for rows as both maps will be updated.
            if (!temp_y_odd)
            {
                if (DmaSystem_AddItemToQueue((uint8_t *)&map_row[0][0], (TILEMAP_ADDR_GAME_MAP+temp_y_wrap), 64, VRAM_INCHIGH, 0))
                {
                    MapSystem_Tilemap_RequestEmergencyRecovery();
                }
                if (DmaSystem_AddItemToQueue((uint8_t *)&map_row[1][0], (TILEMAP_ADDR_GAME_MAP+1024+temp_y_wrap), 64, VRAM_INCHIGH, 0))
                {
                    MapSystem_Tilemap_RequestEmergencyRecovery();
                }
            }
            else
            {
                if (DmaSystem_AddItemToQueue((uint8_t *)&map_row[0][0], (TILEMAP_ADDR_GAME_MAP+32+temp_y_wrap), 64, (VRAM_INCHIGH), 0))
                {
                    MapSystem_Tilemap_RequestEmergencyRecovery();
                }
                if (DmaSystem_AddItemToQueue((uint8_t *)&map_row[1][0], (TILEMAP_ADDR_GAME_MAP+32+1024+temp_y_wrap), 64, (VRAM_INCHIGH), 0))
                {
                    MapSystem_Tilemap_RequestEmergencyRecovery();
                }
            }
        }
    }

    bg_scroll_y_prev.full.high.a = bg_scroll_y.full.high.a;

    return;
}

/*
    Tilemap build a column during live game scrolling

    Can also be invoked to force a full refresh during fblank if needed
*/
/**
 * @brief Constructs a 32-tile vertical column of 16x16 metatiles in `map_column`.
 * 
 * @param p      Pointer to the map grid.
 * @param lut    Pointer to the visual metatile LUT.
 * @param tile_x Metatile X coordinate.
 * @param tile_y Metatile Y coordinate.
 * @param odd    Odd/even tile phase selector.
 * @return VRAM destination offset section.
 */
bool MapSystem_Tilemap_BuildColumn(const uint8_t * p, const uint16_t * lut, int16_t tile_x, int16_t tile_y, bool odd)
{
    // tile_x to determine if it's on the odd or even section of the tilemap.
    // if x = 0:  0 + 16 = 16, 16 >> 4 = 1, 1-1 = 0, 0 & 0x01 = 0
    // if x = 17: 17+ 16 = 33, 33 >> 4 = 2, 2-1 = 1, 1 & 0x01 = 1
    bool temp_section = ((((tile_x + 16) >> 4) - 1 )& 0x0001); // Offset it by 16 so that the values are always positive, then shift right by 4 and take just the lowest bit.
    
    // values here pertain to the buffer
    uint16_t j = ((tile_y + 16) & 0x0f) << 1; // the offset on Y axis

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

    // Make sure X is valid
    if ((tile_x < 0) || (tile_x >= map_extent_tiles_x))
    {
        // X is out of range. Fill the entire column with 0s.
        for (int i = 0; i < 32; i++)
        {
            map_column[i] = 0;
        }
    }
    else if (tile_y >= 0 && tile_y + 15 < map_extent_tiles_y)
    {
        // Fast path: Y is fully in-bounds for all 16 iterations
        int16_t split_index = 15 - (tile_y & 0x0f);
        uint16_t screen_adjustment = (((map_extent_x >> 8) - 1) << 8);

        if (!odd)
        {
            // Even column loop
            for (int i = 0; i <= split_index; i++)
            {
                const uint16_t *lut_ptr = lut + ((*p) << 2);
                map_column[j] = lut_ptr[0];
                map_column[j+1] = lut_ptr[2];
                p += 16;
                j += 2;
            }

            p += screen_adjustment;
            j = 0;

            for (int i = split_index + 1; i < 16; i++)
            {
                const uint16_t *lut_ptr = lut + ((*p) << 2);
                map_column[j] = lut_ptr[0];
                map_column[j+1] = lut_ptr[2];
                p += 16;
                j += 2;
            }
        }
        else
        {
            // Odd column loop
            for (int i = 0; i <= split_index; i++)
            {
                const uint16_t *lut_ptr = lut + ((*p) << 2);
                map_column[j] = lut_ptr[1];
                map_column[j+1] = lut_ptr[3];
                p += 16;
                j += 2;
            }

            p += screen_adjustment;
            j = 0;

            for (int i = split_index + 1; i < 16; i++)
            {
                const uint16_t *lut_ptr = lut + ((*p) << 2);
                map_column[j] = lut_ptr[1];
                map_column[j+1] = lut_ptr[3];
                p += 16;
                j += 2;
            }
        }
    }
    else
    {
        // Slow path fallback: tile_y is near/out of map boundaries
        for (int i = 0; i < 16; i++)
        {
            if ((tile_y < 0) || (tile_y >= map_extent_tiles_y))
            {
                // Y is out of range.
                map_column[j] = 0;
                map_column[j+1] = 0;

                // Do not increment pointer
            }
            else
            {
                // Y is in range.
                int16_t q = (*p) << 2;
                if (!odd)
                {
                    map_column[j] = lut[q];
                    map_column[j+1] = lut[q+2];
                }
                else
                {
                    map_column[j] = lut[q+1];
                    map_column[j+1] = lut[q+3];
                }

                p += 16; // Next row in source data
            }

            tile_y++; // next row in absolute tile count
            if (((tile_y & 0x0f) == 0) && (tile_y > 0))
            {
                // A screen edge has been exceeded
                p += ((((map_extent_x >> 8) - 1) << 8));
            }

            j += 2; // Next row in Y offset within the tilemap
            if (j >= 32)
            {
                j = 0;
            }
        }
    }

    return temp_section;
}

// ditto but for rows
/**
 * @brief Constructs two 32-tile horizontal rows of 16x16 metatiles in `map_row`.
 * 
 * @param p      Pointer to the map grid.
 * @param lut    Pointer to the visual metatile LUT.
 * @param tile_x Metatile X coordinate.
 * @param tile_y Metatile Y coordinate.
 * @param odd    Odd/even tile phase selector.
 */
void MapSystem_Tilemap_BuildRow(const uint8_t * p, const uint16_t * lut, int16_t tile_x, int16_t tile_y, bool odd)
{
    // tile_x to determine if it's on the odd or even section of the tilemap.
    
    uint16_t temp_section = ((((tile_x + 16) >> 4) - 1 )& 0x0001); // Offset it by 16 so that the values are always positive, then shift right by 4 and take just the lowest bit.
    
    // if -7, 15...
    uint16_t k = (tile_x + 32) & 0x1f; // the offset on X axis

    uint16_t temp_internal_section = k >> 4;
    
    uint16_t l = (k & 0xf) << 1; // Use this for writing the tilemap entries, which are only realistically 16 wide

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

    if ((tile_y < 0) || (tile_y >= map_extent_tiles_y))
    {
        // Y is out of range, so both rows can be filled with 0s.
        for (int i = 0; i < 32; i++)
        {
            map_row[0][i] = 0;
            map_row[1][i] = 0;
        }
    }
    else if (tile_x >= 0 && tile_x + 31 < map_extent_tiles_x)
    {
        // Fast path: X is fully in-bounds for all 32 iterations
        int16_t split_index = 15 - (tile_x & 0x0f);

        if (!odd)
        {
            // Loop 1
            for (int i = 0; i <= split_index; i++)
            {
                const uint16_t *lut_ptr = lut + ((*p) << 2);
                map_row[temp_internal_section][l] = lut_ptr[0];
                map_row[temp_internal_section][l+1] = lut_ptr[1];
                p++;
                l += 2;
            }

            p += 240;
            l = 0;
            temp_internal_section ^= 0x0001;

            // Loop 2
            for (int i = split_index + 1; i <= split_index + 16; i++)
            {
                const uint16_t *lut_ptr = lut + ((*p) << 2);
                map_row[temp_internal_section][l] = lut_ptr[0];
                map_row[temp_internal_section][l+1] = lut_ptr[1];
                p++;
                l += 2;
            }

            p += 240;
            l = 0;
            temp_internal_section ^= 0x0001;

            // Loop 3
            for (int i = split_index + 17; i < 32; i++)
            {
                const uint16_t *lut_ptr = lut + ((*p) << 2);
                map_row[temp_internal_section][l] = lut_ptr[0];
                map_row[temp_internal_section][l+1] = lut_ptr[1];
                p++;
                l += 2;
            }
        }
        else
        {
            // Loop 1 (odd)
            for (int i = 0; i <= split_index; i++)
            {
                const uint16_t *lut_ptr = lut + ((*p) << 2);
                map_row[temp_internal_section][l] = lut_ptr[2];
                map_row[temp_internal_section][l+1] = lut_ptr[3];
                p++;
                l += 2;
            }

            p += 240;
            l = 0;
            temp_internal_section ^= 0x0001;

            // Loop 2 (odd)
            for (int i = split_index + 1; i <= split_index + 16; i++)
            {
                const uint16_t *lut_ptr = lut + ((*p) << 2);
                map_row[temp_internal_section][l] = lut_ptr[2];
                map_row[temp_internal_section][l+1] = lut_ptr[3];
                p++;
                l += 2;
            }

            p += 240;
            l = 0;
            temp_internal_section ^= 0x0001;

            // Loop 3 (odd)
            for (int i = split_index + 17; i < 32; i++)
            {
                const uint16_t *lut_ptr = lut + ((*p) << 2);
                map_row[temp_internal_section][l] = lut_ptr[2];
                map_row[temp_internal_section][l+1] = lut_ptr[3];
                p++;
                l += 2;
            }
        }
    }
    else
    {
        // Slow path fallback: tile_x is near/out of map boundaries
        for (int i = 0; i < 32; i++)
        {
            if ((tile_x < 0) || (tile_x >= map_extent_tiles_x))
            {
                // Out of range tile
                map_row[temp_internal_section][l] = 0;
                map_row[temp_internal_section][l+1] = 0;
                // Do not increment pointer
            }
            else
            {
                if (!odd)
                {
                    map_row[temp_internal_section][l] = lut[((*p) << 2)]; // Top left
                    map_row[temp_internal_section][l+1] = lut[((*p) << 2) + 1]; // Top right
                }
                else
                {
                    map_row[temp_internal_section][l] = lut[((*p) << 2) + 2]; // Bottom left
                    map_row[temp_internal_section][l+1] = lut[((*p) << 2) + 3]; // Bottom right
                }
                p++; // Next column in source data
            }

            tile_x++; // next column in absolute tile count
            if (((tile_x & 0x0f) == 0) && (tile_x > 0))
            {
                // A screen edge has been exceeded
                p += 240;
            }

            l += 2; // Next row in X offset within the tilemap row builder
            if (l >= 32)
            {
                l = 0;
                temp_internal_section ^= 0x0001;
            }
        }
    }

    return;
}

