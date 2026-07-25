/*---------------------------------------------------------------------------------

    sprite.h -- definitions for SNES sprites

    Copyright (C) 2012-2024
        Alekmaul

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any
    damages arising from the use of this software.

    Permission is granted to anyone to use this software for any
    purpose, including commercial applications, and to alter it and
    redistribute it freely, subject to the following restrictions:

    1.	The origin of this software must not be misrepresented; you
    must not claim that you wrote the original software. If you use
    this software in a product, an acknowledgment in the product
    documentation would be appreciated but is not required.

    2.	Altered source versions must be plainly marked as such, and
    must not be misrepresented as being the original software.

    3.	This notice may not be removed or altered from any source
    distribution.

---------------------------------------------------------------------------------*/
/*! \file sprite.h
    \brief snes sprites functionality.
*/

/* This file has been slightly adapted for the vbcc compiler:
   - removed __attribute__
*/


#ifndef SNES_SPRITES_INCLUDE
#define SNES_SPRITES_INCLUDE

#include "snes/snestypes.h"

#define OBJ_SIZE8_L16 (0 << 5)  /*!< \brief default OAM size 8x8 (SM) and 16x16 (LG) pix for OBJSEL register */
#define OBJ_SIZE8_L32 (1 << 5)  /*!< \brief default OAM size 8x8 (SM) and 32x32 (LG) pix for OBJSEL register */
#define OBJ_SIZE8_L64 (2 << 5)  /*!< \brief default OAM size 8x8 (SM) and 64x64 (LG) pix for OBJSEL register */
#define OBJ_SIZE16_L32 (3 << 5) /*!< \brief default OAM size 16x16 (SM) and 32x32 (LG) pix for OBJSEL register */
#define OBJ_SIZE16_L64 (4 << 5) /*!< \brief default OAM size 16x16 (SM) and 64x64 (LG) pix for OBJSEL register */
#define OBJ_SIZE32_L64 (5 << 5) /*!< \brief default OAM size 32x32 (SM) and 64x64 (LG) pix for OBJSEL register */

/*!  \brief Sprite Table (from no$sns help file)<br>
Contains data for 128 OBJs. OAM Size is 512+32 Bytes. The first part (512<br>
bytes) contains 128 4-byte entries for each OBJ:<br>
  Byte 0 - X-Coordinate (lower 8bit) (upper 1bit at end of OAM)<br>
  Byte 1 - Y-Coordinate (all 8bits)<br>
  Byte 2 - Tile Number  (lower 8bit) (upper 1bit within Attributes)<br>
  Byte 3 - Attributes<br>
Attributes:<br>
  Bit7    Y-Flip (0=Normal, 1=Mirror Vertically)<br>
  Bit6    X-Flip (0=Normal, 1=Mirror Horizontally)<br>
  Bit5-4  Priority relative to BG (0=Low..3=High)<br>
  Bit3-1  Palette Number (0-7) (OBJ Palette 4-7 can use Color Math via CGADSUB)<br>
  Bit0    Tile Number (upper 1bit)<br>
After above 512 bytes, additional 32 bytes follow, containing 2-bits per OBJ:<br>
  Bit7    OBJ 3 OBJ Size     (0=Small, 1=Large)<br>
  Bit6    OBJ 3 X-Coordinate (upper 1bit)<br>
  Bit5    OBJ 2 OBJ Size     (0=Small, 1=Large)<br>
  Bit4    OBJ 2 X-Coordinate (upper 1bit)<br>
  Bit3    OBJ 1 OBJ Size     (0=Small, 1=Large)<br>
  Bit2    OBJ 1 X-Coordinate (upper 1bit)<br>
  Bit1    OBJ 0 OBJ Size     (0=Small, 1=Large)<br>
  Bit0    OBJ 0 X-Coordinate (upper 1bit)<br>
And so on, next 31 bytes with bits for OBJ4..127. Note: The meaning of the OBJ<br>
Size bit (Small/Large) can be defined in OBSEL Register (Port 2101h).<br>
*/

/*! \def REG_OBSEL
    \brief Object Size and Object Base (W)
    7-5   OBJ Size Selection  (0-5, see below) (6-7=Reserved)
         Val Small  Large
         0 = 8x8    16x16    ;Caution:
         1 = 8x8    32x32    ;In 224-lines mode, OBJs with 64-pixel height
         2 = 8x8    64x64    ;may wrap from lower to upper screen border.
         3 = 16x16  32x32    ;In 239-lines mode, the same problem applies
         4 = 16x16  64x64    ;also for OBJs with 32-pixel height.
         5 = 32x32  64x64
         6 = 16x32  32x64 (undocumented)
         7 = 16x32  32x32 (undocumented)
        (Ie. a setting of 0 means Small OBJs=8x8, Large OBJs=16x16 pixels)
        (Whether an OBJ is "small" or "large" is selected by a bit in OAM)
    4-3   Gap between OBJ 0FFh and 100h (0=None) (4K-word steps) (8K-byte steps)
    2-0   Base Address for OBJ Tiles 000h..0FFh  (8K-word steps) (16K-byte steps)
*/
#define REG_OBSEL (*(vuint8 *)0x2101)

/*! \def REG_OAMADDL
    \def REG_OAMADDH
    \brief OAM Address and Priority Rotation (W)
    15    OAM Priority Rotation  (0=OBJ num 0, 1=OBJ num N) (OBJ with highest priority)
    9-14  Not used
    7-1   OBJ Number num N (for OBJ Priority)   ;bit7-1 are used for two purposes
    8-0   OAM Address   (for OAM read/write)    ;

    This register contains of a 9bit Reload value and a 10bit Address register (plus the
    priority flag). Writing to 2102h or 2103h does change the lower 8bit or upper 1bit of
    the Reload value, and does additionally copy the (whole) 9bit Reload value to the 10bit
    Address register (with address Bit0=0 so next access will be an even address).
    Caution: During rendering, the PPU is destroying the Address register (using it internally
    for whatever purposes), after rendering (at begin of Vblank, ie. at begin of line 225/240,
    but only if not in Forced Blank mode) it reinitializes the Address from the Reload value;
    the same reload occurs also when deactivating forced blank anytime during the first scanline
    of vblank (ie. during line 225/240).
*/
#define REG_OAMADDL (*(vuint8 *)0x2102)
#define REG_OAMADDH (*(vuint8 *)0x2103)
#define REG_OAMADDLH (*(vuint16 *)0x2102)

/*
  1st Access: Lower 8bit (even address)
  2nd Access: Upper 8bit (odd address)
*/
#define REG_OAMDATA (*(vuint8 *)0x2104) /*!< \brief OAM Data Write (W) */
#define REG_RDOAM (*(vuint8 *)0x2138)   /*!< \brief OAM Data Read (R) */

#endif // SNES_SPRITES_INCLUDE
