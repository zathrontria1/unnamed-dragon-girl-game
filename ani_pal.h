extern uint16_t pal_ani_entries[8][2];
extern uint16_t pal_ani_sel;

extern uint16_t pal_cycle_entries[16][16];

void AniSystem_Pal_LoadSubpalette(uint8_t * ptr, uint16_t subpal);
void AniSystem_Pal_LoadCycleSubpalette(uint8_t * ptr, uint16_t subpal);
void AniSystem_Pal_UpdatePalettes(void);
void AniSystem_Pal_PrecalcPaletteChanges(void);

void AniSystem_Pal_CycleSubpalette(uint16_t subpal);
void AniSystem_Pal_CopyCycledSubpaletteToMainSubpalette(uint16_t src_subpal, uint16_t dest_subpal, uint16_t start, uint16_t len);
