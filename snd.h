void SoundInterface_StartSoundEngine();
#if VBCC_ASM == 1
    NO_INLINE void SoundInterface_UploadData(uint8_t * data_ptr, uint16_t len);
#else
    void SoundInterface_UploadData(uint8_t * data_ptr, uint16_t len);
#endif

inline void SoundInterface_PlaySfx(uint8_t sfx_id, int8_t pan);
inline void SoundInterface_PlaySfx_Ex(uint8_t sfx_id, int8_t vol_l, int8_t vol_r, int8_t pitch);
inline void SoundInterface_StopSfx(uint8_t sfx_id);

void SoundInterface_SetDspRegister(uint8_t dsp_reg, uint8_t dsp_data);
void SoundInterface_ResetAPU();

void SoundInterface_UploadSample(struct sample_list_entry * s);

void SoundInterface_UploadInstrumentList(struct sample_list_entry_ins * s);
void SoundInterface_UploadSampleList(struct sample_list_entry * s);
void SoundInterface_SetSampleTune(uint8_t ins_id, uint8_t tune);
void SoundInterface_SetMusicTempo(uint16_t tempo);

void SoundInterface_UploadMusicSequence(struct seq_command * s, uint8_t track);
void SoundInterface_PlayMusic();
void SoundInterface_PauseMusic();
void SoundInterface_StopMusic();

inline void SoundInterface_AcknowledgeBusy();
inline void SoundInterface_AcknowledgeNop();
