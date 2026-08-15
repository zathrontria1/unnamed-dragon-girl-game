#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "snes/console.h"

#include "vars.h"
#include "snd.h"
#include "consts_snd.h"
#include "defs_structs.h"
#include "vars_extern_snd.h"

#if VBCC_ASM == 0

void SoundInterface_UploadData(uint8_t *data_ptr, uint16_t chunk_len)
{
    for (uint16_t i = 0; i < chunk_len; i++)
    {
        REG_APU01 = *data_ptr++;

        uint8_t temp_index_lobyte = (uint8_t)(i);
        REG_APU00 = temp_index_lobyte;

        while (REG_APU00 != temp_index_lobyte)
        {
            ;
        }
    }

    return;
}

void SoundInterface_UploadData_2byte(uint8_t *data_ptr, uint16_t chunk_len)
{
    uint8_t *data_ptr_2 = (uint8_t *)(data_ptr + chunk_len);
    uint8_t temp_internal_counter = 0x00;

    for (uint16_t i = 0; i < chunk_len; i++)
    {
        REG_APU01 = *data_ptr++;
        REG_APU02 = *data_ptr_2++;

        REG_APU00 = temp_internal_counter;

        while (REG_APU00 != temp_internal_counter)
        {
            ;
        }

        temp_internal_counter++;
    }

    return;
}

void SoundInterface_UploadData_2byte_StreamLoopBlock(uint8_t *data_ptr, uint16_t chunk_len)
{
    uint8_t *data_ptr_2 = (uint8_t *)(data_ptr + chunk_len);
    uint8_t temp_internal_counter = 0x00;

    for (uint16_t i = 0; i < chunk_len; i++)
    {
        REG_APU01 = *data_ptr++;

        if (i == 27)
        {
            REG_APU02 = (*data_ptr_2 | 0x03);
            data_ptr_2++;
        }
        else
        {
            REG_APU02 = *data_ptr_2++;
        }

        REG_APU00 = temp_internal_counter;

        while (REG_APU00 != temp_internal_counter)
        {
            ;
        }

        temp_internal_counter++;
    }

    return;
}

/**
 * @brief Handshake verification flow control handler.
 * 
 * Blocks execution until the APU has confirmed it processed the last request.
 * 
 * @param ignore_busy Flag to skip waiting for confirmation in crash scenarios.
 */
void SoundInterface_AcknowledgeBusy(bool ignore_busy)
{
    if (!ignore_busy)
    {
        while (REG_APU00 != snd_current_command_counter)
        {
            ; // Wait for ready. use for things that can't be dropped, but risk of a lock-up
        }
    }
    else
    {
        int wait_counter = 0;

        while (REG_APU00 != snd_current_command_counter)
        {
            wait_counter++; // Wait for ready, within a limit.
            if (wait_counter > 256)
            {
                snd_current_command_counter = REG_APU00;
                break;
            }
        }
    }
    
    return;
}

/**
 * @brief Pushes a NOP sync command to flush the input buffer.
 */
void SoundInterface_AcknowledgeNop()
{
    REG_APU01 = SND_CMD_NOP;
}

/**
 * @brief Sends a direct sound trigger command to the APU without priority checks.
 * 
 * @param sfx_id The ID of the sound effect.
 * @param pan    The audio pan value.
 */
void SoundInterface_PlaySfx_Internal(uint8_t sfx_id, int8_t pan)
{
    SoundInterface_AcknowledgeBusy(true);

    REG_APU02 = sfx_id;
    REG_APU03 = pan;

    REG_APU01 = SND_CMD_SFX_PLAY;

    snd_current_command_counter++;

    return;
}

/**
 * @brief Sends an extended sound trigger command to the APU without priority checks.
 * 
 * @param sfx_id  The ID of the sound effect.
 * @param vol_l   Left channel volume override.
 * @param vol_r   Right channel volume override.
 * @param pitch   Playback pitch modifier.
 */
void SoundInterface_PlaySfx_Ex_Internal(uint8_t sfx_id, int8_t vol_l, int8_t vol_r, int8_t pitch)
{
    SoundInterface_AcknowledgeBusy(true);
    
    REG_APU02 = sfx_id;
    REG_APU03 = pitch;

    REG_APU01 = SND_CMD_SFX_PLAY_EXTEND;

    while (REG_APU01 != SND_CMD_SFX_PLAY_EXTEND)
    {
        ; // Wait for opcode echo.
    }

    REG_APU02 = vol_l;
    REG_APU03 = vol_r;

    REG_APU01 = SND_CMD_SFX_PLAY_EXTEND_VOLDATA;

    snd_current_command_counter++;

    return;
}

/**
 * @brief Sends a stop sound command directly to the APU.
 * 
 * @param sfx_id The ID of the sound effect to silence.
 */
void SoundInterface_StopSfx_Internal(uint8_t sfx_id)
{
    SoundInterface_AcknowledgeBusy(true);

    REG_APU02 = sfx_id;

    REG_APU01 = SND_CMD_SFX_STOP;

    snd_current_command_counter++;

    return;
}

/**
 * @brief Sends a stream stop command directly to the APU.
 */
void SoundInterface_StopStream_Internal()
{
    SoundInterface_AcknowledgeBusy(true);

    REG_APU01 = SND_CMD_STREAM_STOP;

    snd_current_command_counter++;

    return;
}

/**
 * @brief Plays a short sound clip.
 * 
 * @param clip_id The ID of the clip to play.
 */
void SoundInterface_PlayClip(uint16_t clip_id)
{
    if (!snd_settings_enable_voice)
    {
        return;
    }

    SoundInterface_PlayStream(data_stream_table[clip_id].ptr, data_stream_table[clip_id].len, data_stream_table[clip_id].loop);

    return;
}

/**
 * @brief Starts streaming raw audio samples.
 * 
 * @param ptr  Pointer to the source audio stream block.
 * @param len  The total size of the stream.
 * @param loop Whether the stream should restart upon reaching the end.
 */
void SoundInterface_PlayStream(uint8_t * ptr, uint16_t len, bool loop)
{
    if (snd_stream_enable && snd_stream_ptr_start == ptr)
    {
        return;
    }

    SoundInterface_PauseStream();
    
    snd_defercmd_stream_stop_enable = false;

    snd_stream_ptr = ptr;
    snd_stream_ptr_start = ptr;
    snd_stream_current_block = 0;
    
    snd_stream_length = len;
    snd_stream_loop = loop;

    snd_stream_enable = true; // MUST BE SET LAST

    return;
}

/**
 * @brief Resumes active audio stream playback.
 */
void SoundInterface_ResumeStream()
{
    snd_stream_enable = true;

    return;
}

/**
 * @brief Pauses active audio stream playback.
 */
void SoundInterface_PauseStream()
{
    snd_stream_enable = false;

    return;
}

/**
 * @brief Stops active audio stream playback.
 */
void SoundInterface_StopStream()
{
    snd_stream_enable = false;

    snd_stream_ptr = snd_stream_ptr_start;
    snd_stream_current_block = 0;

    snd_defercmd_stream_stop_enable = true;

    return;
}

/**
 * @brief Performs periodic stream data upload transfers to the APU during VBlank.
 */
void SoundInterface_NmiAudioUpload()
{
    SoundInterface_AcknowledgeBusy(true); // hack it here to avoid lockups?

    uint16_t temp_len = 72;

    REG_APU01 = SND_CMD_STREAM_UPLOAD; // Initial

    while (REG_APU01 != SND_CMD_STREAM_UPLOAD)
    {
        ; // Wait for opcode echo.
    }

    // Once the APU replies
    // Begin transfer.
    uint16_t temp_chunk_len = temp_len >> 1;

    if (snd_stream_current_block == 3)
    {
        SoundInterface_UploadData_2byte_StreamLoopBlock(snd_stream_ptr, temp_chunk_len);
    }
    else
    {
        SoundInterface_UploadData_2byte(snd_stream_ptr, temp_chunk_len);
    }

    uint8_t temp_lobyte = (uint8_t)(REG_APU00 + 2);
    REG_APU00 = temp_lobyte; 

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    snd_stream_current_block = ((snd_stream_current_block + 1) & 0x03);

    snd_stream_ptr += 72;
    
    if (snd_stream_ptr >= (snd_stream_ptr_start + snd_stream_length))
    {
        snd_stream_ptr = snd_stream_ptr_start;

        if (!snd_stream_loop)
        {
            // Check if the current stream is already silence
            if (snd_stream_ptr != (uint8_t *)&data_snd_stream_silence)
            {
                // Make sure to play 4 blocks of silence
                SoundInterface_PlayClip(STREAM_SILENCE);
            }
            else
            {
                SoundInterface_StopStream();
            }
        }
    }

    return;
}

/**
 * @brief Resolves all pending deferred sound commands (triggers and silences).
 * 
 * Typically called during interrupts (NMI) to push changes to the APU.
 */
void SoundInterface_RunDeferredCommands()
{
    if (snd_defercmd_sfx_enable)
    {
        if (snd_defercmd_sfx_use_extended_format)
        {
            SoundInterface_PlaySfx_Ex_Internal(snd_defercmd_sfx_id, snd_defercmd_sfx_vol, snd_defercmd_sfx_vol_r, snd_defercmd_sfx_pitch);

            snd_defercmd_sfx_use_extended_format = false;
        }
        else
        {
            SoundInterface_PlaySfx_Internal(snd_defercmd_sfx_id, snd_defercmd_sfx_vol);
        }

        snd_defercmd_sfx_enable = false;

        snd_defercmd_sfx_id = 0; // Set this to invalid
    }

    if (snd_defercmd_sfx_stop_enable)
    {
        SoundInterface_StopSfx_Internal(snd_defercmd_sfx_stop_sfx_id);

        snd_defercmd_sfx_stop_enable = false;

        snd_defercmd_sfx_stop_sfx_id = 0; // Set this to invalid
    }

    if (snd_defercmd_stream_stop_enable)
    {
        SoundInterface_StopStream_Internal();

        snd_defercmd_stream_stop_enable = false;
    }

    return;
}

/**
 * @brief Checks if a sound effect ID has higher priority than currently playing effects.
 * 
 * @param sfx_id The ID of the incoming sound effect.
 * @return True if the incoming effect has priority; otherwise false.
 */
bool SoundInterface_IsHigherPriority(uint8_t sfx_id)
{
    // Just make sure this doesn't get dropped
    if (sfx_id == SFX_ATK_FIRE_BREATH)
    {
        return true;
    }

    // Then make sure item drop sounds play if the current queued sound isn't fire breath
    if ((sfx_id == SFX_DROP_BOUNCE || sfx_id == SFX_DROP_COIN) && (snd_defercmd_sfx_id != SFX_ATK_FIRE_BREATH))
    {
        return true;
    }

    // Then for on-hit noises
    if ((sfx_id == SFX_ATK_PUNCH || sfx_id == SFX_ATK_SPLASH || sfx_id == SFX_ATK_SPLAT_HIT) && (snd_defercmd_sfx_id != SFX_ATK_FIRE_BREATH && snd_defercmd_sfx_id != SFX_DROP_BOUNCE && snd_defercmd_sfx_id != SFX_DROP_COIN))
    {
        return true;
    }

    // If the currently queued sound is these do not let anything else overwrite them
    if ((snd_defercmd_sfx_id == SFX_ATK_FIRE_BREATH) || (snd_defercmd_sfx_id == SFX_UI_CONFIRM) || (snd_defercmd_sfx_id == SFX_MOV_FOOTSTEP))
    {
        return false;
    }

    // Otherwise it's safe
    return true;
}

#endif
