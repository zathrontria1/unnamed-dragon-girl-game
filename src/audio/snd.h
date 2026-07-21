#ifndef SND_H
#define SND_H

#include <stdint.h>
#include <stdbool.h>
#include "defs_structs.h"

extern bool snd_apu_booted;
extern bool snd_settings_mono;

extern uint8_t snd_current_command_counter;

extern bool snd_defercmd_sfx_enable;

extern bool snd_defercmd_sfx_use_extended_format;
extern uint8_t snd_defercmd_sfx_id;
extern int8_t snd_defercmd_sfx_vol;
extern int8_t snd_defercmd_sfx_vol_r;
extern int8_t snd_defercmd_sfx_pitch;

extern bool snd_defercmd_sfx_stop_enable;
extern uint8_t snd_defercmd_sfx_stop_sfx_id;

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
extern uint16_t snd_stream_current_block;

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
    NO_INLINE void SoundInterface_UploadData_2byte_StreamLoopBlock(uint8_t * data_ptr, uint16_t chunk_len);
#else
    void SoundInterface_UploadData_2byte_StreamLoopBlock(uint8_t * data_ptr, uint16_t chunk_len);
#endif

void SoundInterface_PlaySfx_Pre(struct game_object * o, uint8_t sfx_id);

void SoundInterface_PlaySfx(uint8_t sfx_id, int8_t pan);
void SoundInterface_PlaySfx_Internal(uint8_t sfx_id, int8_t pan);
void SoundInterface_PlaySfx_Ex(uint8_t sfx_id, int8_t vol_l, int8_t vol_r, int8_t pitch);
void SoundInterface_PlaySfx_Ex_Internal(uint8_t sfx_id, int8_t vol_l, int8_t vol_r, int8_t pitch);
void SoundInterface_StopSfx(uint8_t sfx_id);
void SoundInterface_StopSfx_Internal(uint8_t sfx_id);

void SoundInterface_RunDeferredCommands();

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

void SoundInterface_PlayClip(uint16_t clip_id); // Wrapper function

void SoundInterface_PlayStream(uint8_t * ptr, uint16_t len, bool loop);
void SoundInterface_ResumeStream();
void SoundInterface_PauseStream();
void SoundInterface_StopStream();

void SoundInterface_NmiAudioUpload();

void SoundInterface_AcknowledgeBusy(bool ignore_busy);
void SoundInterface_AcknowledgeNop();

bool SoundInterface_IsHigherPriority(uint8_t sfx_id);

#endif // SND_H

