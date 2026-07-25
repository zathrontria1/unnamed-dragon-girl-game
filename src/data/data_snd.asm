; For VBCC

    section "_rodata.huge.bindata.audio.engine.0"
    global _data_soundengine_binary
    global _data_soundengine_binary_size
_data_soundengine_binary:
    incbin "sound/sndeng.bin"
_data_soundengine_binary_end:
    _data_soundengine_binary_size:
    dw _data_soundengine_binary_end - _data_soundengine_binary