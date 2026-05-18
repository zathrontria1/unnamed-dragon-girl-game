; Custom defines
; For APUIO0
SND_SIG_CLEAR = $00
SND_SIG_BUSY = $ff

; For APUIO1
SND_CMD_NOP = $00

;SND_CMD_SYNC = $01 ; Not used

; Starts a chain, setting up the length of transfer
; then the sample directory slot.
SND_CMD_DATA_UPLOAD = $02
SND_CMD_DATA_UPLOAD_SLOT = $03
SND_CMD_DATA_SAMPLE_UPLOAD_SAMPLERATE = $04
SND_CMD_DATA_SAMPLE_UPLOAD_LOOPSTART = $05
SND_CMD_DATA_SAMPLE_UPLOAD_ADSR = $06

SND_CMD_DATA_SAMPLE_SET_TUNE = $09

SND_CMD_SFX_PLAY = $10 ; Simple use (pan only)

; Starts a chain, setting up channel volumes and pitch for an SFX
SND_CMD_SFX_PLAY_EXTEND = $11
SND_CMD_SFX_PLAY_EXTEND_VOLDATA = $12

SND_CMD_SFX_STOP = $13 ; Stops all channels with SFX ID

; Music playback to be implemented yet...

SND_CMD_MUS_START = $20
SND_CMD_MUS_PAUSE = $21
SND_CMD_MUS_STOP = $22

SND_CMD_MUS_SET_TEMPO = $23

; upload chain for music (length and track)
SND_CMD_SEQ_UPLOAD = $07
SND_CMD_SEQ_UPLOAD_TRACK = $08

SND_CMD_DIR_RESET = $fd
SND_CMD_DSP_SET = $fe
SND_CMD_SOFTRESET = $ff

SEQ_OPCODE_PLAY_ONESHOT = $80
SEQ_OPCODE_WAIT = $81
SEQ_OPCODE_NOTEPREFIX = $82
SEQ_OPCODE_SETRESTARTPOINT = $83
SEQ_OPCODE_SETLOOPPOINT = $84
SEQ_OPCODE_LOOP = $85
SEQ_OPCODE_RESTART = $ff

;SPC700 Registers
REG_HWTEST1   = $00F0
REG_CONTROL   = $00F1
REG_DSPADDR   = $00F2
REG_DSPDATA   = $00F3
REG_APUIO0    = $00F4
REG_APUIO1    = $00F5
REG_APUIO2    = $00F6
REG_APUIO3    = $00F7
REG_AUXIO4    = $00F8
REG_AUXIO5    = $00F9
REG_T0DIV     = $00FA
REG_T1DIV     = $00FB
REG_T2DIV     = $00FC
REG_T0OUT     = $00FD
REG_T1OUT     = $00FE
REG_T2OUT     = $00FF

DSP_V0VOLL  = $00
DSP_V0VOLR  = $01
DSP_V0PL    = $02
DSP_V0PH    = $03
DSP_V0SRCN  = $04
DSP_V0ADSRL = $05
DSP_V0ADSRH = $06 
DSP_V0GAIN  = $07
DSP_V0ENVX  = $08
DSP_V0OUTX  = $09
DSP_C0      = $0F

DSP_EFB     = $0D
DSP_EVOLL   = $2C
DSP_EVOLR   = $3C
DSP_KON     = $4C
DSP_KOFF    = $5C

DSP_MVOL0L  = $0C
DSP_MVOL0R  = $1C 
DSP_PMON    = $20
DSP_NON     = $3D
DSP_EON     = $40
DSP_ESA     = $6D
DSP_EDL     = $7D

DSP_FLG     = $6C ;RMEN NNNN 	RW 	Flags: soft reset (R), mute all (M), echo disable (E), noise frequency (N). 
DSP_ENDX    = $7C
DSP_DIR     = $5D ;Upper 2 bytes of the brr directory location
