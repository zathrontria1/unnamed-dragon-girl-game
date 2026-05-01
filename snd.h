void snd_start();
#if VBCC_ASM == 1
    NO_INLINE void snd_upload_data(uint8_t * data_ptr, uint16_t len);
#else
    void snd_upload_data(uint8_t * data_ptr, uint16_t len);
#endif

void snd_play_sfx(uint8_t sfx_id, int8_t pan);
void snd_play_sfx_extend(uint8_t sfx_id, int8_t vol_l, int8_t vol_r, int8_t pitch);
void snd_stop_sfx(uint8_t sfx_id);

void snd_set_dsp_reg(uint8_t dsp_reg, uint8_t dsp_data);
void snd_reset();

void snd_upload_sample(struct sample_list_entry * s);

void snd_upload_instrument_list(struct sample_list_entry_ins * s);
void snd_upload_sample_list(struct sample_list_entry * s);
void snd_set_tune(uint8_t ins_id, uint8_t tune);
void snd_set_tempo(uint16_t tempo);

void snd_upload_sequence(struct seq_command * s, uint8_t track);
void snd_music_play();
void snd_music_pause();
void snd_music_stop();

void snd_busy_ack();
void snd_nop_ack();
