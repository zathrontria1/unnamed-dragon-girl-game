#include <snes/console.h>

extern uint8_t system_MVNCodeInWRAM[4];
extern uint8_t system_JMLCodeInWRAM[4];

// Input system
extern uint16_t input_pad0;
extern uint16_t input_pad0_new;

// The custom startup code now wipes the entirety of ZP

void System_DisplayStartupSplash();

void System_Init(void);
void System_Init_CpuRegs(void);
void System_Init_WramFunctions(void);
void System_Init_Graphics(void);
void System_Init_BgScroll(void);
void System_Init_DisplaySettings(uint16_t routine);
void System_Init_TilemapSettings(uint16_t routine);
void System_Init_UiTilemap();

void System_Init_Partial(void);

FORCE_INLINE void System_WaitUntilVblank(void);

void System_GetInput(void);
void System_GetInput_Manual(void);

uint16_t System_CheckController(void);

FORCE_INLINE uint16_t System_CheckKey(enum KEYPAD_BITS k);
FORCE_INLINE uint16_t System_CheckKeyAny();
FORCE_INLINE uint16_t System_CheckKeyHeld(enum KEYPAD_BITS k);

FORCE_INLINE void System_EnableInterrupts(void);
FORCE_INLINE void System_DisableInterrupts(void);
FORCE_INLINE void System_EnableFblankInterrupts(void);

FORCE_INLINE void System_CheckSoftReset(void);
void System_SoftReset(void);
void System_Reset(void);

void System_AlignToVblank();

#if VBCC_ASM == 1
NO_INLINE void System_CopyBlock(__reg("r0/r1") uint8_t * src, __reg("r2/r3") uint8_t * dest, __reg("a") uint16_t len);
#else
FORCE_INLINE void System_CopyBlock(uint8_t * src, uint8_t * dest, uint16_t len);
#endif
