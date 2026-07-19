extern struct cutscene_data * cs_current;
extern uint16_t cs_timer;

extern bool cs_use_second_frame;
extern uint16_t cs_preload_subsection;

void Cs_Loop();
void Cs_StartCutscene();
void Cs_PreloadNextFrame();
