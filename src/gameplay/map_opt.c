#include "snes/console.h"

#include <stdint.h>
#include <stdbool.h>

#include "vars.h"
#include "map.h"

#if VBCC_ASM == 0
/**
 * @brief Decompresses map collision metatiles into the WRAM collision matrix `map_collision_buf`.
 */
void MapSystem_BuildCollisionTable()
{
    const uint8_t * ptr = map_current + 2; 
    const uint8_t * lut = map_lut_col;
    uint16_t stride = map_extent_tiles_x;
    uint16_t screens_x = stride >> 4;
    uint16_t screens_y = map_extent_tiles_y >> 4;
    uint16_t shiftcount_total = map_extent_tiles_x_shiftcount + 4;
    uint16_t sy, sx, ty;

    for (sy = 0; sy < screens_y; sy++)
    {
        uint16_t sy_offset = sy << shiftcount_total;

        for (sx = 0; sx < screens_x; sx++)
        {
            const uint8_t * screen_ptr = ptr + sy_offset + (sx << 8);
            uint16_t dst_base_idx = sy_offset + (sx << 4);

            for (ty = 0; ty < 16; ty++)
            {
                const uint8_t * src_row = screen_ptr + (ty << 4);
                uint8_t * dst_row = &map_collision_buf[dst_base_idx];

                dst_row[0]  = lut[src_row[0]];
                dst_row[1]  = lut[src_row[1]];
                dst_row[2]  = lut[src_row[2]];
                dst_row[3]  = lut[src_row[3]];
                dst_row[4]  = lut[src_row[4]];
                dst_row[5]  = lut[src_row[5]];
                dst_row[6]  = lut[src_row[6]];
                dst_row[7]  = lut[src_row[7]];
                dst_row[8]  = lut[src_row[8]];
                dst_row[9]  = lut[src_row[9]];
                dst_row[10] = lut[src_row[10]];
                dst_row[11] = lut[src_row[11]];
                dst_row[12] = lut[src_row[12]];
                dst_row[13] = lut[src_row[13]];
                dst_row[14] = lut[src_row[14]];
                dst_row[15] = lut[src_row[15]];

                dst_base_idx += stride;
            }
        }
    }

    return;
}
#endif
