extern uint8_t snd_current_command_counter;

extern uint16_t snd_footstep_timeout;
extern uint16_t snd_punch_timeout;
extern uint16_t snd_flame_active;
extern uint16_t snd_flame_playing;
extern uint16_t snd_firecrackle_timeout;

extern uint8_t * snd_stream_ptr;
extern uint8_t * snd_stream_ptr_start;
extern uint16_t snd_stream_length;

extern bool snd_stream_enable;
extern bool snd_stream_loop;
extern bool snd_stream_odd_block;

void SoundInterface_StartSoundEngine();
#if VBCC_ASM == 1
    NO_INLINE void SoundInterface_UploadData(uint8_t * data_ptr, uint16_t len);
#else
    void SoundInterface_UploadData(uint8_t * data_ptr, uint16_t len);
#endif

#if VBCC_ASM == 1
    NO_INLINE void SoundInterface_UploadData_2byte(uint8_t * data_ptr, uint16_t chunk_len);
#else
    void SoundInterface_UploadData_2byte(uint8_t * data_ptr, uint16_t chunk_len);
#endif

#if VBCC_ASM == 1
    NO_INLINE void SoundInterface_UploadData_2byte_StreamOddBlock(uint8_t * data_ptr, uint16_t chunk_len);
#else
    void SoundInterface_UploadData_2byte_StreamOddBlock(uint8_t * data_ptr, uint16_t chunk_len);
#endif

/*
#if VBCC_ASM == 1
    NO_INLINE void SoundInterface_UploadData_3byte(uint8_t * data_ptr, uint16_t chunk_len);
#else
    void SoundInterface_UploadData_3byte(uint8_t * data_ptr, uint16_t chunk_len);
#endif
*/

FORCE_INLINE void SoundInterface_PlaySfx(uint8_t sfx_id, int8_t pan);
FORCE_INLINE void SoundInterface_PlaySfx_Ex(uint8_t sfx_id, int8_t vol_l, int8_t vol_r, int8_t pitch);
FORCE_INLINE void SoundInterface_StopSfx(uint8_t sfx_id);

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

void SoundInterface_PlayStream(uint8_t * ptr, uint16_t len, bool loop);
void SoundInterface_ResumeStream();
void SoundInterface_PauseStream();

void SoundInterface_NmiAudioUpload();

FORCE_INLINE void SoundInterface_AcknowledgeBusy(bool ignore_busy);
FORCE_INLINE void SoundInterface_AcknowledgeNop();
