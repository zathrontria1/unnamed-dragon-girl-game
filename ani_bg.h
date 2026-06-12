// These are locations the game uses to decompress graphics data.
// This means that before running the animated tile decompressor
// any contents in the first 24KB should
// be copied to VRAM
#define ANI_BG_STRIP_ADDR 0x007f0000
#define ANI_BG_FRAME_ADDR 0x007f4000

// Background tile anims
// Water animations are updated per 512byte row
extern uint16_t ani_bg_frame_water; // the 2KB sheet
extern uint16_t ani_bg_row_water; // the 512 byte row section
extern uint8_t * ani_bg_addr_water;
extern uint8_t * ani_bg_addr_water_start;
extern uint16_t ani_bg_dest_water;
ZP extern uint16_t ani_bg_water_dma_ready;

// 64px dedicated section is updated in one go. has to go to the odd frame NMI DMAs.
extern uint16_t ani_bg_frame_tallbg; // the 2KB sheet
extern uint8_t * ani_bg_addr_tallbg;
extern uint8_t * ani_bg_addr_tallbg_start;
extern uint16_t ani_bg_dest_tallbg;
ZP extern uint16_t ani_bg_tallbg_dma_ready;

void AniSystem_BgTile_UpdateStrip(void);
void AniSystem_BgTile_UpdateFrame(void);

void AniSystem_BgTile_SetStripPointer(uint8_t * ptr);
void AniSystem_BgTile_SetFramePointer(uint8_t * ptr);

void AniSystem_BgTile_Setup(uint8_t * ptr_strip, uint8_t * ptr_frame);
