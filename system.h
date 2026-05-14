#include <snes/console.h>

extern uint8_t system_MVNCodeInWRAM[4];
extern uint8_t system_JMLCodeInWRAM[4];

//void system_init_zp(void); // The custom startup code now wipes the entirety of ZP
void system_init_regs(void);
void system_init_wram_functions(void);
void system_display_splash();
void system_init(void);
void system_init_partial(void);
void system_init_graphics(void);
void system_reset_bg_scroll_regs(void);
void system_init_display(uint16_t routine);
void system_setup_tilemap_display(uint16_t routine);
void system_reset_ui_tilemap();

inline void system_wait_vblank(void);
void system_poll_input(void);
inline uint16_t system_check_for_key(enum KEYPAD_BITS k);
inline uint16_t system_check_for_key_hold(enum KEYPAD_BITS k);
inline void system_interrupt_enable(void);
inline void system_interrupt_disable(void);
inline void system_check_for_soft_reset(void);
void system_reset(void);
