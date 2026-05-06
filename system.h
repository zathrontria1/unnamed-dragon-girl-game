extern uint8_t system_MVNCodeInWRAM[4];
extern uint8_t system_JMLCodeInWRAM[4];

void system_init_zp(void);
void system_init_regs(void);
void system_display_splash();
void system_init(void);
void system_init_graphics(void);
void system_reset_bg_scroll_regs(void);
void system_init_display(uint16_t routine);
void system_setup_tilemap_display(uint16_t routine);
void system_reset_ui_tilemap();

void system_wait_vblank(void);
inline void system_poll_input(void);
inline uint16_t system_check_for_key(enum KEYPAD_BITS k);
inline uint16_t system_check_for_key_hold(enum KEYPAD_BITS k);
void system_interrupt_enable(void);
void system_interrupt_disable(void);
inline void system_check_for_soft_reset(void);
void system_reset(void);


