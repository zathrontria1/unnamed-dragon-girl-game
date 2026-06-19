extern struct cutscene_data * cs_current;
extern uint16_t cs_timer;

void CsEngine_Loop();
void CsEngine_StartCutscene();
void CsEngine_PreloadNext();
