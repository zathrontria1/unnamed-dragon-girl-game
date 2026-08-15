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
uint8_t snd_settings_volume; // 0-127. Do not use the highest bit.
bool snd_settings_enable_bgm;
bool snd_settings_enable_sfx;
bool snd_settings_enable_voice;

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
    if (!snd_settings_enable_sfx)
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
    if (!snd_settings_enable_sfx)
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

    if (temp > 0xfffe)
    {
        temp = 0xfffe;
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
 * @brief Adjusts the sequence playback tempo.
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

    uint32_t val = 120000ul / tempo;
    uint16_t temp_interval = 1;

    while (val >= 255)
    {
        val >>= 1;
        temp_interval <<= 1;
    }

    uint8_t temp_t2timer = (uint8_t)val;

    if ((temp_t2timer == 0) && (val != 0))
    {
        temp_t2timer = 255;
    }

    if (temp_interval > 255)
    {
        temp_interval = 255;
    }

    REG_APU03 = (uint8_t)temp_interval;
    REG_APU02 = temp_t2timer;
    REG_APU01 = SND_CMD_MUS_SET_TEMPO; // Initial

    while (REG_APU01 != SND_CMD_MUS_SET_TEMPO)
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
void SoundInterface_UploadMusicSequence(struct seq_command * s, uint8_t track)
{
    // Scan the sequence to get its length first
    uint16_t temp_len = 4; // Include the terminator

    struct seq_command * temp_ptr = s;

    while (temp_ptr->opcode != SEQ_OPCODE_RESTART)
    {
        temp_len += 4;
        temp_ptr++;
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
    if (!snd_settings_enable_bgm)
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


