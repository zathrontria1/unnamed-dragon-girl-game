/*---------------------------------------------------------------------------------

    Copyright (C) 2012-2017
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
/*! \file dma.h
\brief Wrapper functions for direct memory access hardware

*/
#ifndef SNES_DMA_INCLUDE
#define SNES_DMA_INCLUDE

#include <snes/snestypes.h>

#include <snes/background.h>
#include <snes/sprite.h>
#include <snes/video.h>

/*!	\brief Bit defines for the HDMA channels */
#define HDMA_CHANNEL0			(1 << 0)
#define HDMA_CHANNEL1			(1 << 1)
#define HDMA_CHANNEL2			(1 << 2)
#define HDMA_CHANNEL3			(1 << 3)
#define HDMA_CHANNEL4			(1 << 4)
#define HDMA_CHANNEL5			(1 << 5)
#define HDMA_CHANNEL6			(1 << 6)
#define HDMA_CHANNEL7			(1 << 7)
#define HDMA_CHANNELALL			(HDMA_CHANNEL0 | HDMA_CHANNEL1 | HDMA_CHANNEL2 | HDMA_CHANNEL3 | HDMA_CHANNEL4 | HDMA_CHANNEL5 | HDMA_CHANNEL6 | HDMA_CHANNEL7)

/*!	\brief Bit defines for the window area main screen effect */
#define MSWIN_BG1               (1 << 0) /*!< \brief Main Screen BG1 disable background */
#define MSWIN_BG2               (1 << 1) /*!< \brief Main Screen BG2 disable background */
#define MSWIN_BG3               (1 << 2) /*!< \brief Main Screen BG3 disable background */
#define MSWIN_BG4               (1 << 3) /*!< \brief Main Screen BG4 disable background */

#define MSWIN1_BG1MSKOUT        (1 << 0) /*!< \brief Window 1 area BG1 inside (0) outside(1) */
#define MSWIN1_BG1MSKENABLE     (2 << 0) /*!< \brief Window 1 area BG1 enable */
#define MSWIN2_BG1MSKOUT        (1 << 2) /*!< \brief Window 2 area BG1 inside (0) outside(1) */
#define MSWIN2_BG1MSKENABLE     (2 << 2) /*!< \brief Window 2 area BG1 enable */
#define MSWIN1_BG2MSKOUT        (1 << 4) /*!< \brief Window 1 area BG2 inside (0) outside(1) */
#define MSWIN1_BG2MSKENABLE     (2 << 4) /*!< \brief Window 1 area BG2 enable */
#define MSWIN2_BG2MSKOUT        (1 << 4) /*!< \brief Window 2 area BG2 inside (0) outside(1) */
#define MSWIN2_BG2MSKENABLE     (2 << 4) /*!< \brief Window 2 area BG2 enable */
#define MSWIN1_BG3MSKOUT        (1 << 0) /*!< \brief Window 1 area BG3 inside (0) outside(1) */
#define MSWIN1_BG3MSKENABLE     (2 << 0) /*!< \brief Window 1 area BG3 enable */
#define MSWIN2_BG3MSKOUT        (1 << 2) /*!< \brief Window 2 area BG3 inside (0) outside(1) */
#define MSWIN2_BG3MSKENABLE     (2 << 2) /*!< \brief Window 2 area BG3 enable */
#define MSWIN1_BG4MSKOUT        (1 << 4) /*!< \brief Window 1 area BG4 inside (0) outside(1) */
#define MSWIN1_BG4MSKENABLE     (2 << 4) /*!< \brief Window 1 area BG4 enable */
#define MSWIN2_BG4MSKOUT        (1 << 4) /*!< \brief Window 2 area BG4 inside (0) outside(1) */
#define MSWIN2_BG4MSKENABLE     (2 << 4) /*!< \brief Window 2 area BG4 enable */

/*!	\brief Bit defines for the DMA control registers */
#define DMA_ENABLE 1

/*! \def REG_MDMAEN

    \brief Select General Purpose DMA Channel(s) and Start Transfer (W)
     7-0   General Purpose DMA Channel 7-0 Enable (0=Disable, 1=Enable)

    When writing a non-zero value to this register, general purpose DMA
    will be started immediately (after a few clk cycles). The CPU is paused
    during the transfer. The transfer can be interrupted by H-DMA transfers.
    If more than 1 bit is set in MDMAEN, then the separate transfers will be
    executed in WHICH? priority order. The MDMAEN bits are cleared automatically
    at transfer completion.
    Do not use channels for GP-DMA which are activated as H-DMA in HDMAEN.

*/
#define REG_MDMAEN (*(vuint8 *)0x420B)

/*! \def REG_HDMAEN

    \brief Select H-Blank DMA (H-DMA) Channel(s) (W)
     7-0   H-DMA Channel 7-0 Enable (0=Disable, 1=Enable)
*/
#define REG_HDMAEN (*(vuint8 *)0x420C)

/*! \def REG_DMAP0
    \def REG_DMAP1	(*(vuint8*)0x4310)
    \def REG_DMAP2	(*(vuint8*)0x4320)
    \def REG_DMAP3	(*(vuint8*)0x4330)
    \def REG_DMAP4	(*(vuint8*)0x4340)
    \def REG_DMAP5	(*(vuint8*)0x4350)
    \def REG_DMAP6	(*(vuint8*)0x4360)
    \def REG_DMAP7	(*(vuint8*)0x4370)
    \brief DMA/HDMA Parameters (R/W)
    7     Transfer Direction (0=A:CPU to B:I/O, 1=B:I/O to A:CPU)
    6     Addressing Mode    (0=Direct Table, 1=Indirect Table)    (HDMA only)
    5     Not used (R/W) (unused and unchanged by all DMA and HDMA)
    4-3   A-BUS Address Step  (0=Increment, 2=Decrement, 1/3=Fixed) (DMA only)
    2-0   Transfer Unit Select (0-4=see below, 5-7=Reserved)

    DMA Transfer Unit Selection:
    Mode  Bytes              B-Bus 21xxh Address   ;Usage Examples...
    0  =  Transfer 1 byte    xx                    ;eg. for WRAM (port 2180h)
    1  =  Transfer 2 bytes   xx, xx+1              ;eg. for VRAM (port 2118h/19h)
    2  =  Transfer 2 bytes   xx, xx                ;eg. for OAM or CGRAM
    3  =  Transfer 4 bytes   xx, xx,   xx+1, xx+1  ;eg. for BGnxOFS, M7x
    4  =  Transfer 4 bytes   xx, xx+1, xx+2, xx+3  ;eg. for BGnSC, Window, APU..
    5  =  Transfer 4 bytes   xx, xx+1, xx,   xx+1  ;whatever purpose, VRAM maybe
    6  =  Transfer 2 bytes   xx, xx                ;same as mode 2
    7  =  Transfer 4 bytes   xx, xx,   xx+1, xx+1  ;same as mode 3
*/
#define REG_DMAP0 (*(vuint8 *)0x4300)
#define REG_DMAP1 (*(vuint8 *)0x4310)
#define REG_DMAP2 (*(vuint8 *)0x4320)
#define REG_DMAP3 (*(vuint8 *)0x4330)
#define REG_DMAP4 (*(vuint8 *)0x4340)
#define REG_DMAP5 (*(vuint8 *)0x4350)
#define REG_DMAP6 (*(vuint8 *)0x4360)
#define REG_DMAP7 (*(vuint8 *)0x4370)

/*! \def REG_BBAD0
    \def REG_BBAD1
    \def REG_BBAD2
    \def REG_BBAD3
    \def REG_BBAD4
    \def REG_BBAD5
    \def REG_BBAD6
    \def REG_BBAD7
    \brief DMA/HDMA I/O-Bus Address (PPU-Bus aka B-Bus) (R/W)
    For both DMA and HDMA:
    7-0   B-Bus Address (selects an I/O Port which is mapped to 2100h-21FFh)
*/
#define REG_BBAD0 (*(vuint8 *)0x4301)
#define REG_BBAD1 (*(vuint8 *)0x4311)
#define REG_BBAD2 (*(vuint8 *)0x4321)
#define REG_BBAD3 (*(vuint8 *)0x4331)
#define REG_BBAD4 (*(vuint8 *)0x4341)
#define REG_BBAD5 (*(vuint8 *)0x4351)
#define REG_BBAD6 (*(vuint8 *)0x4361)
#define REG_BBAD7 (*(vuint8 *)0x4371)

/*! \def REG_A1T0LH
    \def REG_A1T1LH
    \def REG_A1T2LH
    \def REG_A1T3LH
    \def REG_A1T4LH
    \def REG_A1T5LH
    \def REG_A1T6LH
    \def REG_A1T7LH

    \def REG_A1B0
    \def REG_A1B1
    \def REG_A1B2
    \def REG_A1B3
    \def REG_A1B4
    \def REG_A1B5
    \def REG_A1B6
    \def REG_A1B7
    \brief 	A1TxL - HDMA Table Start Address (low) / DMA Current Addr (low) (R/W)
    A1TxH - HDMA Table Start Address (hi) / DMA Current Addr (hi) (R/W)
    A1Bx - HDMA Table Start Address (bank) / DMA Current Addr (bank) (R/W)

    For normal DMA:
    23-16  CPU-Bus Data Address Bank (constant, not incremented/decremented)
    15-0   CPU-Bus Data Address (incremented/decremented/fixed, as selected)

    For HDMA:
    23-16  CPU-Bus Table Address Bank (constant, bank number for 43x8h/43x9h)
    15-0   CPU-Bus Table Address      (constant, reload value for 43x8h/43x9h)
*/
#define REG_A1T0LH (*(vuint16 *)0x4302)
#define REG_A1T1LH (*(vuint16 *)0x4312)
#define REG_A1T2LH (*(vuint16 *)0x4322)
#define REG_A1T3LH (*(vuint16 *)0x4332)
#define REG_A1T4LH (*(vuint16 *)0x4342)
#define REG_A1T5LH (*(vuint16 *)0x4352)
#define REG_A1T6LH (*(vuint16 *)0x4362)
#define REG_A1T7LH (*(vuint16 *)0x4372)

#define REG_A1B0 (*(vuint8 *)0x4304)
#define REG_A1B1 (*(vuint8 *)0x4314)
#define REG_A1B2 (*(vuint8 *)0x4324)
#define REG_A1B3 (*(vuint8 *)0x4334)
#define REG_A1B4 (*(vuint8 *)0x4344)
#define REG_A1B5 (*(vuint8 *)0x4354)
#define REG_A1B6 (*(vuint8 *)0x4364)
#define REG_A1B7 (*(vuint8 *)0x4374)

/*! \def REG_DAS0LH
    \def REG_DAS1LH
    \def REG_DAS2LH
    \def REG_DAS3LH
    \def REG_DAS4LH
    \def REG_DAS5LH
    \def REG_DAS6LH
    \def REG_DAS7LH
    \brief Indirect HDMA Address (low) / DMA Byte-Counter (low) (R/W)
    DASxL - Indirect HDMA Address (low) / DMA Byte-Counter (low) (R/W)
    DASxH - Indirect HDMA Address (hi) / DMA Byte-Counter (hi) (R/W)
    43x7h - DASBx - Indirect HDMA Address (bank) (R/W)

    For normal DMA:
    23-16  Not used
    15-0   Number of bytes to be transferred (1..FFFFh=1..FFFFh, or 0=10000h)
    (This is really a byte-counter; with a 4-byte "Transfer Unit", len=5 would
    transfer one whole Unit, plus the first byte of the second Unit.)
    (The 16bit value is decremented during transfer, and contains 0000h on end.)
*/
#define REG_DAS0LH (*(vuint16 *)0x4305)
#define REG_DAS1LH (*(vuint16 *)0x4315)
#define REG_DAS2LH (*(vuint16 *)0x4325)
#define REG_DAS3LH (*(vuint16 *)0x4335)
#define REG_DAS4LH (*(vuint16 *)0x4345)
#define REG_DAS5LH (*(vuint16 *)0x4355)
#define REG_DAS6LH (*(vuint16 *)0x4365)
#define REG_DAS7LH (*(vuint16 *)0x4375)

#define REG_DASB0 (*(vuint8 *)0x4307)
#define REG_DASB1 (*(vuint8 *)0x4317)
#define REG_DASB2 (*(vuint8 *)0x4327)
#define REG_DASB3 (*(vuint8 *)0x4337)
#define REG_DASB4 (*(vuint8 *)0x4347)
#define REG_DASB5 (*(vuint8 *)0x4357)
#define REG_DASB6 (*(vuint8 *)0x4367)
#define REG_DASB7 (*(vuint8 *)0x4377)

#define REG_WMDATA (*(vuint8 *)0x2180)
#define REG_WMADDL (*(vuint8 *)0x2181)
#define REG_WMADDM (*(vuint8 *)0x2182)
#define REG_WMADDLM (*(vuint16 *)0x2181)
#define REG_WMADDH (*(vuint8 *)0x2183)

#endif // SNES_DMA_INCLUDE
