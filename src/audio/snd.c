#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "snes/console.h"

#include "vars.h"

#include "map.h"

#include "snd.h"
#include "consts_snd.h"

bool snd_apu_booted;

// User-settable settings.
bool snd_settings_mono;
uint8_t snd_settings_volume_master; // 0-127. Do not use the highest bit.
uint8_t snd_settings_volume_bgm;
uint8_t snd_settings_volume_sfx;
uint8_t snd_settings_volume_voice;

uint8_t snd_current_command_counter; // Used to check if the SPC is ready for a new command

// Used to implement a deferred SFX system
bool snd_defercmd_sfx_enable;

bool snd_defercmd_sfx_use_extended_format;
uint8_t snd_defercmd_sfx_id;
int8_t snd_defercmd_sfx_vol;
int8_t snd_defercmd_sfx_vol_r;
int8_t snd_defercmd_sfx_pitch;

bool snd_defercmd_sfx_stop_enable;
uint8_t snd_defercmd_sfx_stop_sfx_id;

bool snd_defercmd_stream_stop_enable;

uint16_t snd_footstep_timeout;
uint16_t snd_punch_timeout;
uint16_t snd_flame_active;
uint16_t snd_flame_playing;
uint16_t snd_firecrackle_timeout;

uint8_t * snd_stream_ptr;
uint8_t * snd_stream_ptr_start;
uint16_t snd_stream_length;
bool snd_stream_enable;
bool snd_stream_loop;
uint16_t snd_stream_current_block;

/*
    Convention outside the stock IPL:

    APU0: acknowledgement pipe, always check this before execution
    APU1: opcode (used to echo to check if a command was sent correctly)
    APU2-3: operands (2 bytes)

    Opcodes cannot be 0x00, as that is the "pipe empty" state.

    Once an opcode has been transferred successfully, later commands can repurpose 
    APU IO ports, enabling up to 3 bytes sent at a go. 

    APU0 should be used as echo/sync byte even with multi byte transfers.

    For block transfers, transfer ends are implicit.
*/

/**
 * @brief Boots the SPC700 sound engine.
 * 
 * Handshakes with the stock SPC700 boot ROM, uploads the driver binary payload, 
 * and jumps to the driver entry point.
 */
void SoundInterface_StartSoundEngine()
{
    // Wait for SPC to become ready
    while (REG_APU0001 != 0xbbaa)
    {
        ;
    }

    // Write start address
    REG_APU02 = 0x00;
    REG_APU03 = 0x03; // 0x0300

    // Write non-zero
    REG_APU01 = 0xcc;

    // Write 0xcc
    REG_APU00 = 0xcc; 

    while (REG_APU00 != 0xcc)
    {
        ; // Wait for acknowledgement
    }

    // Address is now set
    SoundInterface_UploadData((uint8_t *)&data_soundengine_binary, data_soundengine_binary_size);

    // Start the engine
    // Write start address
    REG_APU02 = 0x00;
    REG_APU03 = 0x03; // 0x0300

    // Write zero
    REG_APU01 = 0x00;

    // Write the read value here plus 2
    uint8_t temp_lobyte = (uint8_t)(REG_APU00 + 2);
    REG_APU00 = temp_lobyte; 

    while (REG_APU00 != temp_lobyte)
    {
        ; // Wait for acknowledgement
    }

    REG_APU00 = 0x00;

    snd_apu_booted = true;

    return;
}




/**
 * @brief Enqueues a sound effect trigger centered on a game object's screen position.
 * 
 * Calculates panning values based on the object's offset relative to the camera view.
 * 
 * @param o      Pointer to the game object causing the sound.
 * @param sfx_id The ID of the sound effect.
 */
void SoundInterface_PlaySfx_Pre(struct game_object * o, uint8_t sfx_id)
{        
    if (!snd_settings_volume_sfx)
    {
        return;
    }

    int temp_snd_pan;

    if (!snd_settings_mono)
    {
        temp_snd_pan = o->pos.x.lh.h - 128 - bg_scroll_x.full.high.a;
        if (temp_snd_pan < -127)
        {
            temp_snd_pan = -127;
        }
        else if (temp_snd_pan > 127)
        {
            temp_snd_pan = 127;
        }
    }
    else
    {
        temp_snd_pan = 0;
    }

    SoundInterface_PlaySfx(sfx_id, temp_snd_pan);

    return;
}

/**
 * @brief Triggers a standard sound effect at a given pan value.
 * 
 * Checks priorities before sending.
 * 
 * @param sfx_id The ID of the sound effect.
 * @param pan    The audio pan value (-128 to 127).
 */
void SoundInterface_PlaySfx(uint8_t sfx_id, int8_t pan)
{
    if (!SoundInterface_IsHigherPriority(sfx_id))
    {
        return;
    }

    snd_defercmd_sfx_enable = false; // Always disable the sfx enablement while changing variables
    
    snd_defercmd_sfx_use_extended_format = false;

    snd_defercmd_sfx_id = sfx_id;
    snd_defercmd_sfx_vol = pan;

    snd_defercmd_sfx_enable = true;

    return;
}




/**
 * @brief Triggers an extended sound effect with left volume, right volume, and pitch modifications.
 * 
 * Checks priorities before sending.
 * 
 * @param sfx_id  The ID of the sound effect.
 * @param vol_l   Left channel volume override.
 * @param vol_r   Right channel volume override.
 * @param pitch   Playback pitch modifier.
 */
void SoundInterface_PlaySfx_Ex(uint8_t sfx_id, int8_t vol_l, int8_t vol_r, int8_t pitch)
{
    if (!snd_settings_volume_sfx)
    {
        return;
    }
    
    if (!SoundInterface_IsHigherPriority(sfx_id))
    {
        return;
    }

    snd_defercmd_sfx_enable = false; // Always disable the sfx enablement while changing variables

    snd_defercmd_sfx_use_extended_format = true;

    snd_defercmd_sfx_id = sfx_id;

    if (snd_settings_mono)
    {
        int8_t mono_vol = (vol_l > vol_r) ? vol_l : vol_r;
        vol_l = mono_vol;
        vol_r = mono_vol;
    }
    
    snd_defercmd_sfx_vol = vol_l;
    snd_defercmd_sfx_vol_r = vol_r;
    snd_defercmd_sfx_pitch = pitch;

    snd_defercmd_sfx_enable = true;

    return;
}



/**
 * @brief Requests silencing a specific sound effect.
 * 
 * @param sfx_id The ID of the sound effect to silence.
 */
void SoundInterface_StopSfx(uint8_t sfx_id)
{
    snd_defercmd_sfx_stop_enable = false; // Always disable the sfx enablement while changing variables

    snd_defercmd_sfx_stop_sfx_id = sfx_id;

    snd_defercmd_sfx_stop_enable = true;

    return;
}




/**
 * @brief Sets a specific DSP internal register value on the sound processor.
 * 
 * @param dsp_reg  The target DSP register address index.
 * @param dsp_data The byte value to write.
 */
void SoundInterface_SetDspRegister(uint8_t dsp_reg, uint8_t dsp_data)
{
    SoundInterface_AcknowledgeBusy(false);

    REG_APU02 = dsp_reg;
    REG_APU03 = dsp_data;

    REG_APU01 = SND_CMD_DSP_SET;

    while (REG_APU01 != SND_CMD_DSP_SET)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/*
    Helper function to set master volume. This is a convenience function that sets both left and right master volume registers to the same value.

    mvol must be 127 or lower.
    */
void SoundInterface_SetMasterVolume(uint8_t mvol)
{
    if (mvol > 127)
    {
        mvol = 127;
    }
    
    SoundInterface_SetDspRegister(0x0c, mvol);
    SoundInterface_SetDspRegister(0x1c, mvol);

    return;
}

void SoundInterface_SetMusicVolume(uint8_t vol)
{
    if (vol > 127)
    {
        vol = 127;
    }

    SoundInterface_AcknowledgeBusy(false);

    REG_APU02 = vol;
    REG_APU01 = SND_CMD_SET_MUSIC_VOL;

    while (REG_APU01 != SND_CMD_SET_MUSIC_VOL)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

void SoundInterface_SetSfxVolume(uint8_t vol)
{
    if (vol > 127)
    {
        vol = 127;
    }

    SoundInterface_AcknowledgeBusy(false);

    REG_APU02 = vol;
    REG_APU01 = SND_CMD_SET_SFX_VOL;

    while (REG_APU01 != SND_CMD_SET_SFX_VOL)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

void SoundInterface_SetVoiceVolume(uint8_t vol)
{
    if (vol > 127)
    {
        vol = 127;
    }

    SoundInterface_AcknowledgeBusy(false);

    REG_APU02 = vol;
    REG_APU01 = SND_CMD_SET_VOICE_VOL;

    while (REG_APU01 != SND_CMD_SET_VOICE_VOL)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/**
 * @brief Soft resets the APU state and blocks until the sound chip is ready to accept commands.
 */
void SoundInterface_ResetAPU()
{
    SoundInterface_AcknowledgeBusy(false);

    REG_APU01 = SND_CMD_SOFTRESET;

    snd_current_command_counter = 0;

    while (REG_APU01 != SND_CMD_SOFTRESET)
    {
        ; // Wait for opcode echo.
    }

    return;
}

/**
 * @brief Uploads a BRR audio sample struct to the sound engine.
 * 
 * @param s Pointer to the sample definition structure.
 */
void SoundInterface_UploadSample(struct sample_list_entry * s)
{
    SoundInterface_AcknowledgeBusy(false);

    uint16_t temp_len = s->len;
    
    // Sanity check the length first. If the length is not even, pad it
    if ((s->len & 0x0001) != 0x0000)
    {
        temp_len++;
    }

    REG_APU0203 = temp_len;

    REG_APU01 = SND_CMD_DATA_SAMPLE_UPLOAD; // Initial

    uint32_t temp;

    if (s->ticks != 0)
    {
        temp = s->ticks;
    }
    else
    {
        // Undo the value change first
        uint32_t temp_realsamplerate = ((((uint32_t)(s->sample_rate))* 32000l) >> 12l);
        temp = ((((s->len * 3l) << 4l) / 9l) * (32000l / temp_realsamplerate)) / 1600l;
    }

    if (temp > 0xffff)
    {
        temp = 0xffff;
    }

    while (REG_APU01 != SND_CMD_DATA_SAMPLE_UPLOAD)
    {
        ; // Wait for opcode echo.
    }

    REG_APU0203 = (uint16_t)temp;
    REG_APU01 = SND_CMD_DATA_SAMPLE_UPLOAD_TICK; // Phase two

    while (REG_APU01 != SND_CMD_DATA_SAMPLE_UPLOAD_TICK)
    {
        ; // Wait for opcode echo.
    }
    
    REG_APU02 = s->id;
    REG_APU01 = SND_CMD_DATA_SAMPLE_UPLOAD_SLOT; // Phase two

    while (REG_APU01 != SND_CMD_DATA_SAMPLE_UPLOAD_SLOT)
    {
        ; // Wait for opcode echo.
    }

    REG_APU0203 = s->sample_rate;
    REG_APU01 = SND_CMD_DATA_SAMPLE_UPLOAD_SAMPLERATE; // Phase three

    while (REG_APU01 != SND_CMD_DATA_SAMPLE_UPLOAD_SAMPLERATE)
    {
        ; // Wait for opcode echo.
    }

    uint16_t * loop_point = (uint16_t*)s->data_ptr;
    REG_APU0203 = *loop_point;
    REG_APU01 = SND_CMD_DATA_SAMPLE_UPLOAD_LOOPSTART; // Phase three

    while (REG_APU01 != SND_CMD_DATA_SAMPLE_UPLOAD_LOOPSTART)
    {
        ; // Wait for opcode echo.
    }

    REG_APU0203 = s->adsr;
    REG_APU01 = SND_CMD_DATA_SAMPLE_UPLOAD_ADSR; // Phase four

    while (REG_APU01 != SND_CMD_DATA_SAMPLE_UPLOAD_ADSR)
    {
        ; // Wait for opcode echo.
    }

    // Once the APU replies
    // Begin transfer.

    uint8_t * ptr = s->data_ptr;

    ptr += 2;
    
    uint16_t temp_chunk_len = temp_len >> 1;

    SoundInterface_UploadData_2byte(ptr, temp_chunk_len);

    uint8_t temp_lobyte = (uint8_t)(REG_APU00 + 2);
    REG_APU00 = temp_lobyte; 

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/**
 * @brief Uploads an array of BRR sample definitions to the APU.
 * 
 * @param s Pointer to the start of the sample definition array.
 */
void SoundInterface_UploadSampleList(struct sample_list_entry * s)
{
    while (s->len != 0)
    {
        SoundInterface_UploadSample(s);

        s++;
    }

    return;
}

/**
 * @brief Uploads an instrument structure list defining sound properties to the APU.
 * 
 * @param s Pointer to the start of the instrument array definition.
 */
void SoundInterface_UploadInstrumentList(struct sample_list_entry_ins * s)
{
    while (s->len != 0)
    {
        SoundInterface_UploadSample((struct sample_list_entry *)s); // cast it
        SoundInterface_SetSampleTune(s->id, s->tune);

        s++;
    }

    return;
}

/**
 * @brief Locks in the post-SFX sample boundary in the sound engine.
 */
void SoundInterface_LockSfxBoundary()
{
    SoundInterface_AcknowledgeBusy(false);

    REG_APU01 = SND_CMD_LOCK_SFX;

    while (REG_APU01 != SND_CMD_LOCK_SFX)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/**
 * @brief Clears song instruments and rewinds the sample heap to the SFX boundary.
 */
void SoundInterface_ResetSongInstruments()
{
    SoundInterface_AcknowledgeBusy(false);

    REG_APU01 = SND_CMD_MUS_INS_RESET;

    while (REG_APU01 != SND_CMD_MUS_INS_RESET)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/**
 * @brief Updates the default pitch tuning index for a specific instrument ID.
 * 
 * @param ins_id The instrument ID index.
 * @param tune   The tuning byte value.
 */
void SoundInterface_SetSampleTune(uint8_t ins_id, uint8_t tune)
{
    SoundInterface_AcknowledgeBusy(false);

    REG_APU03 = tune;
    REG_APU02 = ins_id;
    REG_APU01 = SND_CMD_DATA_SAMPLE_SET_TUNE; // Initial

    while (REG_APU01 != SND_CMD_DATA_SAMPLE_SET_TUNE)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/**
 * @brief Adjusts the sequence playback tempo using Impulse Tracker tick timing.
 * 
 * @param tempo The tempo rate value in BPM.
 */
void SoundInterface_SetMusicTempo(uint16_t tempo)
{
    SoundInterface_AcknowledgeBusy(false);

    if (tempo == 0)
    {
        tempo = 1; // Avoid division by zero
    }

    uint32_t val = (20000ul + (tempo >> 1)) / tempo;
    uint16_t temp_interval = 1;

    while (val > 255)
    {
        val >>= 1;
        temp_interval <<= 1;
    }

    uint8_t temp_t1timer = (uint8_t)val;

    if (temp_t1timer == 0)
    {
        temp_t1timer = 1;
    }

    if (temp_interval > 255)
    {
        temp_interval = 255;
    }

    REG_APU03 = (uint8_t)temp_interval;
    REG_APU02 = temp_t1timer;
    REG_APU01 = SND_CMD_MUS_SET_TEMPO; // Initial

    while (REG_APU01 != SND_CMD_MUS_SET_TEMPO)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/**
 * @brief Adjusts the sequence playback speed (ticks per row).
 * 
 * @param speed The number of ticks per row (typically 1..32, default 6).
 */
void SoundInterface_SetMusicSpeed(uint8_t speed)
{
    if (speed == 0)
    {
        speed = 1;
    }

    SoundInterface_AcknowledgeBusy(false);

    REG_APU02 = speed;
    REG_APU01 = SND_CMD_MUS_SET_SPEED; // Initial

    while (REG_APU01 != SND_CMD_MUS_SET_SPEED)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

void SoundInterface_SetOutputMode(uint8_t mode)
{
    SoundInterface_AcknowledgeBusy(false);

    REG_APU02 = mode;
    REG_APU01 = SND_CMD_MUS_SET_OUTPUTMODE; // Initial

    while (REG_APU01 != SND_CMD_MUS_SET_OUTPUTMODE)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/**
 * @brief Uploads music sequences to the sound engine's tracks.
 * 
 * @param s     Pointer to the sequence commands array.
 * @param track The target track channel.
 */
void SoundInterface_UploadMusicSequence(const uint8_t * s, uint8_t track)
{
    // Scan the sequence to get its length first, advancing according to opcodes
    const uint8_t * temp_ptr = s;
    uint16_t max_sub_offset = 0;

    while (*temp_ptr != SEQ_OPCODE_RESTART)
    {
        uint8_t op = *temp_ptr;
        if (op < 0x80) // 1-byte note key-on
        {
            temp_ptr += 1;
        }
        else if (op <= 0x8f) // 1-byte short wait
        {
            temp_ptr += 1;
        }
        else
        {
            switch (op)
            {
                case SEQ_OPCODE_WAIT_EXT:     // 2 bytes: opcode + ticks
                case SEQ_OPCODE_SET_INS:      // 2 bytes: opcode + ins_id
                case SEQ_OPCODE_SET_DURATION: // 2 bytes: opcode + ticks
                case SEQ_OPCODE_PLAY_DRUM:    // 2 bytes: opcode + drum_id
                case SEQ_OPCODE_SET_LOOP:     // 2 bytes: opcode + count
                case SEQ_OPCODE_SET_SPEED:    // 2 bytes: opcode + speed
                case SEQ_OPCODE_SET_TEMPO:    // 2 bytes: opcode + tempo
                    temp_ptr += 2;
                    break;
                case SEQ_OPCODE_SET_VOL:      // 3 bytes: opcode + vol_l + vol_r
                case SEQ_OPCODE_SET_ADSR:     // 3 bytes: opcode + adsr_l + adsr_h
                case SEQ_OPCODE_SET_PORTA:    // 3 bytes: opcode + note + speed
                    temp_ptr += 3;
                    break;
                case SEQ_OPCODE_CALL_SUB:     // 3 bytes: opcode + offset_l + offset_h
                {
                    uint16_t off = (uint16_t)temp_ptr[1] | ((uint16_t)temp_ptr[2] << 8);
                    if (off > max_sub_offset)
                    {
                        max_sub_offset = off;
                    }
                    temp_ptr += 3;
                    break;
                }
                case SEQ_OPCODE_LOOP:         // 1 byte
                case SEQ_OPCODE_SET_RESTART:  // 1 byte
                case SEQ_OPCODE_NOTE_CUT:     // 1 byte
                case SEQ_OPCODE_RET:          // 1 byte
                    temp_ptr += 1;
                    break;
                default:
                    temp_ptr += 1;
                    break;
            }
        }
    }
    temp_ptr += 1; // Include SEQ_OPCODE_RESTART (0xFF)

    // If subroutines were called, scan the final subroutine block up to its SEQ_OPCODE_RET
    if (max_sub_offset > 0)
    {
        temp_ptr = s + max_sub_offset;
        while (*temp_ptr != SEQ_OPCODE_RET)
        {
            uint8_t op = *temp_ptr;
            if (op < 0x80)
            {
                temp_ptr += 1;
            }
            else if (op <= 0x8f)
            {
                temp_ptr += 1;
            }
            else
            {
                switch (op)
                {
                    case SEQ_OPCODE_WAIT_EXT:
                    case SEQ_OPCODE_SET_INS:
                    case SEQ_OPCODE_SET_DURATION:
                    case SEQ_OPCODE_PLAY_DRUM:
                    case SEQ_OPCODE_SET_LOOP:
                    case SEQ_OPCODE_SET_SPEED:
                    case SEQ_OPCODE_SET_TEMPO:
                        temp_ptr += 2;
                        break;
                    case SEQ_OPCODE_SET_VOL:
                    case SEQ_OPCODE_SET_ADSR:
                    case SEQ_OPCODE_SET_PORTA:
                    case SEQ_OPCODE_CALL_SUB:
                        temp_ptr += 3;
                        break;
                    default:
                        temp_ptr += 1;
                        break;
                }
            }
        }
        temp_ptr += 1; // Include SEQ_OPCODE_RET (0x9D)
    }

    uint16_t temp_len = (uint16_t)(temp_ptr - s);
    // Align length to 2-byte boundary (accept slight 1-byte overrun)
    if (temp_len & 1)
    {
        temp_len++;
    }

    SoundInterface_AcknowledgeBusy(false);
    
    REG_APU0203 = temp_len; // length is always even
    REG_APU01 = SND_CMD_SEQ_UPLOAD; // Initial

    while (REG_APU01 != SND_CMD_SEQ_UPLOAD)
    {
        ; // Wait for opcode echo.
    }

    REG_APU02 = track;
    REG_APU01 = SND_CMD_SEQ_UPLOAD_TRACK; // Phase two

    while (REG_APU01 != SND_CMD_SEQ_UPLOAD_TRACK)
    {
        ; // Wait for opcode echo.
    }

    // Once the APU replies
    // Begin transfer.
    uint8_t * ptr = (uint8_t *)s;
    
    uint16_t temp_chunk_len = temp_len >> 1;

    SoundInterface_UploadData_2byte(ptr, temp_chunk_len);

    uint8_t temp_lobyte = (uint8_t)(REG_APU00 + 2);
    REG_APU00 = temp_lobyte; 

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/**
 * @brief Starts sequence music playback.
 */
void SoundInterface_PlayMusic()
{
    if (!snd_settings_volume_bgm)
    {
        return;
    }

    SoundInterface_AcknowledgeBusy(false);

    REG_APU01 = SND_CMD_MUS_START;

    while (REG_APU01 != SND_CMD_MUS_START)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/**
 * @brief Pauses sequence music playback.
 */
void SoundInterface_PauseMusic()
{
    SoundInterface_AcknowledgeBusy(false);

    REG_APU01 = SND_CMD_MUS_PAUSE;

    while (REG_APU01 != SND_CMD_MUS_PAUSE)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

/**
 * @brief Stops sequence music playback.
 */
void SoundInterface_StopMusic()
{
    SoundInterface_AcknowledgeBusy(false);

    REG_APU01 = SND_CMD_MUS_STOP;

    while (REG_APU01 != SND_CMD_MUS_STOP)
    {
        ; // Wait for opcode echo.
    }

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}


