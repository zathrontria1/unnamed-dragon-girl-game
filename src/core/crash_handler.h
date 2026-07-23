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

enum crashhandler_error_code
{
	CRASHHANDLER_ERROR_UNKNOWN = 0,
	CRASHHANDLER_ERROR_MAIN_LOOP_NULL,
	CRASHHANDLER_ERROR_PLAYER_INSTANTIATION,
	CRASHHANDLER_ERROR_SPAWNER_LIST_INSTANTIATION,
	CRASHHANDLER_ERROR_INTERACTABLE_LIST_INSTANTIATION,
	CRASHHANDLER_ERROR_MAP_TOO_LARGE,
	CRASHHANDLER_ERROR_NPC_OUT_OF_BOUNDS,
	CRASHHANDLER_ERROR_SPAWNER_OUT_OF_BOUNDS,
	CRASHHANDLER_ERROR_INVALID_EVENT_FLAG,
	CRASHHANDLER_ERROR_INTERACTABLE_OUT_OF_BOUNDS,
	CRASHHANDLER_ERROR_INVALID_NS_DOOR_WARP,
	CRASHHANDLER_ERROR_INVALID_EW_DOOR_WARP,
	CRASHHANDLER_ERROR_COUNT
};

extern uint8_t crashhandler_error_code;

void System_CrashHandler();
void System_CrashHandler_EmulationMode();
void System_CrashHandler_Followup();
