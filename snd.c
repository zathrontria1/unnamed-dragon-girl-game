#include <snes/console.h>

#include "vars.h"

#include "snd.h"
#include "consts_snd.h"

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

void snd_start()
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
    snd_upload_data((uint8_t *)&data_soundengine_binary, 2048);

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

#ifdef __VBCC__
    NO_INLINE void snd_upload_data(uint8_t * data_ptr, uint16_t len)
#else
    void snd_upload_data(uint8_t * data_ptr, uint16_t len)
#endif
{
    // i compare with size of the binary blob
    // May need to change i to exactly 0 and no offset on compared size
    // when implementing actual driver.

    #ifdef __VBCC__
         __asm(
            "\tsta r0\n" // Data pointer
            "\tstx r0+2\n"

            "\tlda 4,s\n" // Length of transfer
            "\tbeq .end_sound\n"
            "\tsta r2\n" 

            "\tphy\n"
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

            "\tply\n"
            "\ta16\n"
            "\trep #$20\n"
        ".end_sound:\n");
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

void snd_busy_ack()
{
    while (REG_APU00 != SND_SIG_CLEAR)
    {
        ; // Wait for ready
    }

    return;
}

void snd_nop_ack()
{
    REG_APU01 = SND_CMD_NOP;

    while (REG_APU01 != SND_CMD_NOP)
    {
        ; // Wait for acknowledgement
    }

    return;
}

void snd_play_sfx(uint8_t sfx_id, int8_t pan)
{
    snd_busy_ack();

    REG_APU02 = sfx_id;
    REG_APU03 = pan;

    REG_APU01 = SND_CMD_SFX_PLAY;

    while (REG_APU01 != SND_CMD_SFX_PLAY)
    {
        ; // Wait for opcode echo.
    }

    snd_nop_ack();

    return;
}

void snd_play_sfx_extend(uint8_t sfx_id, int8_t vol_l, int8_t vol_r, int8_t pitch)
{
    snd_busy_ack();
    
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

    while (REG_APU01 != SND_CMD_SFX_PLAY_EXTEND_VOLDATA)
    {
        ; // Wait for opcode echo.
    }

    REG_APU01 = SND_CMD_NOP;

    snd_nop_ack();

    return;
}

// stop an SFX
void snd_stop_sfx(uint8_t sfx_id)
{
    snd_busy_ack();

    REG_APU02 = sfx_id;

    REG_APU01 = SND_CMD_SFX_STOP;

    while (REG_APU01 != SND_CMD_SFX_STOP)
    {
        ; // Wait for opcode echo.
    }

    snd_nop_ack();

    return;
}

/*
    Set up a DSP register from main CPU by calling this function
*/

void snd_set_dsp_reg(uint8_t dsp_reg, uint8_t dsp_data)
{
    snd_busy_ack();

    REG_APU02 = dsp_reg;
    REG_APU03 = dsp_data;

    REG_APU01 = SND_CMD_DSP_SET;

    while (REG_APU01 != SND_CMD_DSP_SET)
    {
        ; // Wait for opcode echo.
    }

    snd_nop_ack();

    return;
}

/*
    Restarts the SPC into the IPL loader.

    Run this if soft resetting, otherwise game will hang during startup.
*/
void snd_reset()
{
    snd_busy_ack();

    REG_APU01 = SND_CMD_SOFTRESET;

    while (REG_APU01 != SND_CMD_SOFTRESET)
    {
        ; // Wait for opcode echo.
    }

    return;
}

void snd_upload_sample(struct sample_list_entry * s)
{
    snd_busy_ack();
    
    REG_APU0203 = s->len;
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
    
    snd_upload_data(ptr, s->len);
    //snd_upload_data_3byte(data_ptr, len);

    uint8_t temp_lobyte = (uint8_t)(REG_APU00 + 2);
    REG_APU00 = temp_lobyte; 

    snd_nop_ack();

    return;
}

void snd_upload_sample_list(struct sample_list_entry * s)
{
    while (s->len != 0)
    {
        snd_upload_sample(s);

        s++;
    }

    return;
}

void snd_upload_instrument_list(struct sample_list_entry_ins * s)
{
    while (s->len != 0)
    {
        snd_upload_sample((struct sample_list_entry *)s); // cast it
        snd_set_tune(s->id, s->tune);

        s++;
    }

    return;
}

void snd_set_tune(uint8_t ins_id, uint8_t tune)
{
    snd_busy_ack();

    REG_APU03 = tune;
    REG_APU02 = ins_id;
    REG_APU01 = SND_CMD_DATA_SAMPLE_SET_TUNE; // Initial

    while (REG_APU01 != SND_CMD_DATA_SAMPLE_SET_TUNE)
    {
        ; // Wait for opcode echo.
    }

    snd_nop_ack();

    return;
}

/*
    Convert a BPM into the equivalent in 
    timer ticks + intervals
*/
void snd_set_tempo(uint16_t tempo)
{
    snd_busy_ack();

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

    snd_nop_ack();

    return;
}

void snd_upload_sequence(struct seq_command * s, uint8_t track)
{
    // Scan the sequence to get its length first
    uint16_t temp_len = 4; // Include the terminator

    struct seq_command * temp_ptr = s;

    while (temp_ptr->opcode != SEQ_OPCODE_RESTART)
    {
        temp_len += 4;
        temp_ptr++;
    }

    snd_busy_ack();
    
    REG_APU0203 = temp_len;
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
    
    snd_upload_data(ptr, temp_len);

    uint8_t temp_lobyte = (uint8_t)(REG_APU00 + 2);
    REG_APU00 = temp_lobyte; 

    snd_nop_ack();

    return;
}

void snd_music_play()
{
    snd_busy_ack();

    REG_APU01 = SND_CMD_MUS_START;

    while (REG_APU01 != SND_CMD_MUS_START)
    {
        ; // Wait for opcode echo.
    }

    return;
}

void snd_music_pause()
{
    snd_busy_ack();

    REG_APU01 = SND_CMD_MUS_PAUSE;

    while (REG_APU01 != SND_CMD_MUS_PAUSE)
    {
        ; // Wait for opcode echo.
    }

    return;
}

void snd_music_stop()
{
    snd_busy_ack();

    REG_APU01 = SND_CMD_MUS_STOP;

    while (REG_APU01 != SND_CMD_MUS_STOP)
    {
        ; // Wait for opcode echo.
    }

    return;
}