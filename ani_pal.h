extern NEAR uint16_t pal_ani_entries[8][2]; // Just enough for the magic circle
extern uint16_t pal_ani_sel;

void AniSystem_Pal_LoadSubpalette(uint8_t * ptr, uint16_t subpal);
void AniSystem_Pal_UpdatePalettes(void);
void AniSystem_Pal_PrecalcPaletteChanges(void);

void AniSystem_Pal_CycleSubpalette(uint16_t subpal);
