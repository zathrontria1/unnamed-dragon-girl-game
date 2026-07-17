extern uint16_t crashhandler_a;
extern uint16_t crashhandler_x;
extern uint16_t crashhandler_y;

extern uint8_t crashhandler_flags;

extern uint32_t crashhandler_pc;
extern uint16_t crashhandler_sp;

extern uint16_t crashhandler_directpage;
extern uint8_t crashhandler_databank;

extern uint32_t crashhandler_regs[16];
extern uint32_t crashhandler_regs_float[4];
extern uint16_t crashhandler_stack[16];

extern uint32_t crashhandler_emulation_mode;

void System_CrashHandler();
void System_CrashHandler_EmulationMode();
void System_CrashHandler_Followup();
