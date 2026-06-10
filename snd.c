#include <snes/console.h>

#include "vars.h"

#include "snd.h"
#include "consts_snd.h"

uint8_t snd_current_command_counter; // Used to check if the SPC is ready for a new command

// Used to implement a deferred SFX system
bool snd_defercmd_sfx_enable;

bool snd_defercmd_sfx_use_extended_format;
uint8_t snd_defercmd_sfx_id;
int8_t snd_defercmd_sfx_vol;
int8_t snd_defercmd_sfx_vol_r;
int8_t snd_defercmd_sfx_pitch;

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

    When the SPC is ready to receive a new command,
    APU0 will be set to 0xff
    APU1-3 will be set to 0x000000
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
    REG_APU03 = 0x02; // 0x0200

    // Write non-zero
    REG_APU01 = 0xcc;

    // Write 0xcc
    REG_APU00 = 0xcc; 

    while (REG_APU00 != 0xcc)
    {
        ; // Wait for acknowledgement
    }

    // Address is now set
    SoundInterface_UploadData((uint8_t *)&data_soundengine_binary, 2560);

    // Start the engine
    // Write start address
    REG_APU02 = 0x00;
    REG_APU03 = 0x02; // 0x0200

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

    return;
}

#if VBCC_ASM == 1
    NO_INLINE void SoundInterface_UploadData(uint8_t * data_ptr, uint16_t chunk_len)
#else
    void SoundInterface_UploadData(uint8_t * data_ptr, uint16_t chunk_len)
#endif
{
    // i compare with size of the binary blob
    // May need to change i to exactly 0 and no offset on compared size
    // when implementing actual driver.

    #if VBCC_ASM == 1
         __asm(
            "\tsta r0\n" // Data pointer
            "\tstx r0+2\n"

            "\tlda 4,s\n" // Length of transfer
            "\tbeq .end_sound\n"
            "\tsta r2\n" 
            
            "\tldy #0\n"

            "\tsep #$20\n"
            "\ta8\n"
        ".write_apu_byte:\n"
            "\tlda [r0],y\n"
            "\tsta 8513\n"
            "\ttya\n"
            "\tsta 8512\n"
        ".check_ack:\n"
            "\tcmp 8512\n"
            "\tbne .check_ack\n"

            "\tiny\n"
            "\tcpy r2\n"
            "\tbcc .write_apu_byte\n"

            "\ta16\n"
            "\trep #$20\n"
        ".end_sound:\n"
        );
    #else
        
        for (uint16_t i = 0; i < len; i++)
        {
            REG_APU01 = *data_ptr++;

            uint8_t temp_index_lobyte = (uint8_t)(i);
            REG_APU00 = temp_index_lobyte;

            while (REG_APU00 != temp_index_lobyte)
            {
                ; // Wait for acknowledgement
            }
        }
    #endif
    return;
}

#if VBCC_ASM == 1
    NO_INLINE void SoundInterface_UploadData_2byte(uint8_t * data_ptr, uint16_t chunk_len)
#else
    void SoundInterface_UploadData_2byte(uint8_t * data_ptr, uint16_t chunk_len)
#endif
{
    // i compare with size of the binary blob
    // May need to change i to exactly 0 and no offset on compared size
    // when implementing actual driver.

    #if VBCC_ASM == 1
         __asm(
            "\ta16\n"
            "\tx16\n"

            "\tsta r0\n" // Data pointer 0
            "\tstx r0+2\n"
            "\tstx r3+2\n"

            "\tlda 4,s\n" // Length of transfer
            "\tbeq .end_sound\n"
            "\tsta r2\n" 
            "\tclc\n" 
            "\tadc r0\n"
            "\tsta r3\n" // Data pointer 1. Note that it won't cross pointers.
            
            "\tldy #0\n"

            "\tsep #$20\n"
            "\ta8\n"
        ".write_apu_byte:\n"
            "\tlda [r0],y\n"
            "\tsta 8513\n"
            "\tlda [r3],y\n"
            "\tsta 8514\n"
            "\ttya\n"
            "\tsta 8512\n"
        ".check_ack:\n"
            "\tcmp 8512\n"
            "\tbne .check_ack\n"

            "\tiny\n"
            "\tcpy r2\n"
            "\tbcc .write_apu_byte\n"

            "\ta16\n"
            "\trep #$20\n"
        ".end_sound:\n"
        );
    #else
        uint8_t * data_ptr_2 = (uint8_t *)(data_ptr + chunk_len);
        uint8_t temp_internal_counter = 0x00;

        for (uint16_t i = 0; i < chunk_len; i += 2)
        {
            REG_APU01 = *data_ptr++;
            REG_APU02 = *data_ptr_2++;

            REG_APU00 = temp_internal_counter;

            while (REG_APU00 != temp_internal_counter)
            {
                ; // Wait for acknowledgement
            }

            temp_internal_counter++;
        }
    #endif
    return;
}

#if VBCC_ASM == 1
    NO_INLINE void SoundInterface_UploadData_2byte_StreamLoopBlock(uint8_t * data_ptr, uint16_t chunk_len)
#else
    void SoundInterface_UploadData_2byte_StreamLoopBlock(uint8_t * data_ptr, uint16_t chunk_len)
#endif
{
    // i compare with size of the binary blob
    // May need to change i to exactly 0 and no offset on compared size
    // when implementing actual driver.

    // Special version to make sure that the 63th byte of an odd block has the loop flag set.

    #if VBCC_ASM == 1
         __asm(
            "\ta16\n"
            "\tx16\n"

            "\tsta r0\n" // Data pointer 0
            "\tstx r0+2\n"
            "\tstx r3+2\n"

            "\tlda 4,s\n" // Length of transfer
            "\tbeq .end_sound\n"
            "\tsta r2\n" 
            "\tclc\n" 
            "\tadc r0\n"
            "\tsta r3\n" // Data pointer 1. Note that it won't cross pointers.
            
            "\tldy #0\n"

            "\tsep #$20\n"
            "\ta8\n"
        ".write_apu_byte:\n"
            "\tlda [r0],y\n"
            "\tsta 8513\n"
            "\tlda [r3],y\n" // When Y is 27, byte is 63th here.
            "\tcpy #27\n"
            "\tbne .not_63th_byte\n"
            "\tora #$03\n" // set low 2 bits
            ".not_63th_byte:\n"
            "\tsta 8514\n"
            "\ttya\n"
            "\tsta 8512\n"
        ".check_ack:\n"
            "\tcmp 8512\n"
            "\tbne .check_ack\n"

            "\tiny\n"
            "\tcpy r2\n"
            "\tbcc .write_apu_byte\n"

            "\ta16\n"
            "\trep #$20\n"
        ".end_sound:\n"
        );
    #else
        uint8_t * data_ptr_2 = (uint8_t *)(data_ptr + chunk_len);
        uint8_t temp_internal_counter = 0x00;

        for (uint16_t i = 0; i < chunk_len; i += 2)
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
                ; // Wait for acknowledgement
            }

            temp_internal_counter++;
        }
    #endif
    return;
}

/*
#if VBCC_ASM == 1
    NO_INLINE void SoundInterface_UploadData_3byte(uint8_t * data_ptr, uint16_t len)
#else
    void SoundInterface_UploadData_3byte(uint8_t * data_ptr, uint16_t chunk_len)
#endif
{
    // Note: the length passed is the chunk length.

    // i compare with size of the binary blob
    // May need to change i to exactly 0 and no offset on compared size
    // when implementing actual driver.

    #if VBCC_ASM == 1
         __asm(
            "\ta16\n"
            "\tx16\n"

            "\tphy\n"

            "\tpei (r0)\n"
            "\tpei (r1)\n"
            "\tpei (r2)\n"
            "\tpei (r3)\n"
            "\tpei (r4)\n"
            "\tpei (r5)\n"
            "\tpei (r6)\n"

            "\tsta r0\n" // Data pointer 0
            "\tstx r0+2\n" // Bank bytes
            "\tstx r3+2\n"
            "\tstx r5+2\n"

            "\tlda 20,s\n" // Length of transfer
            "\tbeq .end_sound\n"
            "\tsta r2\n" 
            "\tclc\n" 
            "\tadc r0\n"
            "\tsta r3\n" // Data pointer 1. Note that it won't cross pointers.
            "\tadc r2\n"
            "\tsta r5\n" // Data pointer 2
            
            "\tldy #0\n"

            "\tsep #$20\n"
            "\ta8\n"
        ".write_apu_byte:\n"
            "\tlda [r0],y\n"
            "\tsta 8513\n"
            "\tlda [r3],y\n"
            "\tsta 8514\n"
            "\tlda [r5],y\n"
            "\tsta 8515\n"
            "\ttya\n"
            "\tsta 8512\n"
        ".check_ack:\n"
            "\tcmp 8512\n"
            "\tbne .check_ack\n"

            "\tiny\n"
            "\tcpy r2\n"
            "\tbcc .write_apu_byte\n"

            "\ta16\n"
            "\trep #$20\n"
        ".end_sound:\n"
            "\tply\n"
            "\tsty r6\n"
            "\tply\n"
            "\tsty r5\n"
            "\tply\n"
            "\tsty r4\n"
            "\tply\n"
            "\tsty r3\n"
            "\tply\n"
            "\tsty r2\n"
            "\tply\n"
            "\tsty r1\n"
            "\tply\n"
            "\tsty r0\n"

            "\tply\n"
        );
    #else
        uint8_t * data_ptr_2 = (uint8_t *)(data_ptr + chunk_len);
        uint8_t * data_ptr_3 = (uint8_t *)(data_ptr + chunk_len + chunk_len);
        uint8_t temp_internal_counter = 0x00;

        for (uint16_t i = 0; i < chunk_len; i += 2)
        {
            REG_APU01 = *data_ptr++;
            REG_APU02 = *data_ptr_2++;
            REG_APU03 = *data_ptr_3++;

            REG_APU00 = temp_internal_counter;

            while (REG_APU00 != temp_internal_counter)
            {
                ; // Wait for acknowledgement
            }

            temp_internal_counter++;
        }
    #endif
    return;
}
    */

FORCE_INLINE void SoundInterface_AcknowledgeBusy(bool ignore_busy)
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

FORCE_INLINE void SoundInterface_AcknowledgeNop()
{
    REG_APU01 = SND_CMD_NOP;
}

FORCE_INLINE void SoundInterface_PlaySfx(uint8_t sfx_id, int8_t pan)
{
    SoundInterface_AcknowledgeBusy(true);

    REG_APU02 = sfx_id;
    REG_APU03 = pan;

    REG_APU01 = SND_CMD_SFX_PLAY;

    snd_current_command_counter++;

    return;
}

FORCE_INLINE void SoundInterface_PlaySfx_Ex(uint8_t sfx_id, int8_t vol_l, int8_t vol_r, int8_t pitch)
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

// stop an SFX
FORCE_INLINE void SoundInterface_StopSfx(uint8_t sfx_id)
{
    SoundInterface_AcknowledgeBusy(true);

    REG_APU02 = sfx_id;

    REG_APU01 = SND_CMD_SFX_STOP;

    snd_current_command_counter++;

    return;
}

/*
    Set up a DSP register from main CPU by calling this function
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
    Restarts the SPC into the IPL loader.

    Run this if soft resetting, otherwise game will hang during startup.
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

void SoundInterface_UploadSample(struct sample_list_entry * s)
{
    SoundInterface_AcknowledgeBusy(false);

    uint16_t temp_len = s->len;
    
    // Sanity check the length first. If the length is not even, pad it
    if (s->len & 0x0001 != 0x0000)
    {
        temp_len++;
    }

    /*while (temp_len % 3 != 0)
    {
        temp_len++;
    }*/
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

    while (REG_APU01 != SND_CMD_DATA_SAMPLE_UPLOAD)
    {
        ; // Wait for opcode echo.
    }

    //REG_APU03 = s->ticks; // now implied
    REG_APU03 = (uint8_t)temp;
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
    //uint16_t temp_chunk_len = temp_len / 3;

    //SoundInterface_UploadData(ptr, s->len);
    SoundInterface_UploadData_2byte(ptr, temp_chunk_len);
    //SoundInterface_UploadData_3byte(ptr, temp_chunk_len); // Provide the adjusted length

    uint8_t temp_lobyte = (uint8_t)(REG_APU00 + 2);
    REG_APU00 = temp_lobyte; 

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

void SoundInterface_UploadSampleList(struct sample_list_entry * s)
{
    while (s->len != 0)
    {
        SoundInterface_UploadSample(s);

        s++;
    }

    return;
}

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

/*
    Convert a BPM into the equivalent in 
    timer ticks + intervals
*/
void SoundInterface_SetMusicTempo(uint16_t tempo)
{
    SoundInterface_AcknowledgeBusy(false);

    uint8_t temp_t2timer;
    uint16_t temp_interval = 1;

    float temp_rows_per_second = ((float)(tempo << 2) / 60.0f) ; // mul 4 div 60f

    float temp_t2timer_f = 8000.0f;

    while ((uint32_t)temp_t2timer_f > 255)
    {
        temp_t2timer_f = 8000.0f / temp_rows_per_second;

        if ((uint32_t)temp_t2timer_f < 255)
        {
            temp_t2timer = (uint8_t)temp_t2timer_f;

            if ((temp_t2timer == 0) && (temp_t2timer_f != 0))
            {
                temp_t2timer = 255;
            }

            if (temp_interval > 255)
            {
                temp_interval = 255;
            }
            break;
        }

        temp_interval <<= 1;
        temp_rows_per_second *= 2.0f;
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

    //SoundInterface_UploadData(ptr, temp_len);
    SoundInterface_UploadData_2byte(ptr, temp_chunk_len);

    uint8_t temp_lobyte = (uint8_t)(REG_APU00 + 2);
    REG_APU00 = temp_lobyte; 

    snd_current_command_counter++;

    SoundInterface_AcknowledgeNop();

    return;
}

void SoundInterface_PlayMusic()
{
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

/*
    Set streaming engine variables
*/

/*
    Wrapper for PlayStream
*/
void SoundInterface_PlayClip(uint16_t clip_id)
{
    SoundInterface_PlayStream(data_stream_table[clip_id].ptr, data_stream_table[clip_id].len, data_stream_table[clip_id].loop);

    return;
}

void SoundInterface_PlayStream(uint8_t * ptr, uint16_t len, bool loop)
{
    SoundInterface_PauseStream();
    
    snd_stream_ptr = ptr + (snd_stream_current_block * 72);
    snd_stream_ptr_start = ptr;
    
    snd_stream_length = len;
    snd_stream_loop = loop;

    // Reuse the current block indicator to prevent stream errors
    //snd_stream_current_block = 0;
    snd_stream_enable = true; // MUST BE SET LAST

    return;
}

void SoundInterface_ResumeStream()
{
    snd_stream_enable = true;

    return;
}

void SoundInterface_PauseStream()
{
    snd_stream_enable = false;

    return;
}

void SoundInterface_StopStream()
{
    snd_stream_enable = false;

    snd_stream_ptr = snd_stream_ptr_start;
    snd_stream_current_block = 0;

    return;
}

// Call after NMI to upload 72 bytes
void SoundInterface_NmiAudioUpload()
{
    SoundInterface_AcknowledgeBusy(false);

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

// Function used to play queued sfx automatically
void SoundInterface_PlayDeferredSfx()
{

    return;
}