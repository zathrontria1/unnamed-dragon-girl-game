; Constant HDMA Indirect Pointer Tables for SNES
; Defined in ROM for vasm/vlink

    section "_rodata.far.hdma.bgpalette"
    global _hdma_bgpalette_tables
_hdma_bgpalette_tables:
    byte $80 | 112
    word <_hdma_bgpalette_data
    byte $80 | 112
    word <(_hdma_bgpalette_data + 448)
    byte 0
    word 0

    section "_rodata.far.hdma.windowbg"
    global _hdma_windowbackground_tables
_hdma_windowbackground_tables:
    ; Textbox ver
    byte 80
    word <_hdma_windowbackground_data
    byte 80
    word <_hdma_windowbackground_data
    byte $80 | 52
    word <_hdma_windowbackground_data
    byte 0
    word 0

    ; Fullscreen ver
    byte $80 | 112
    word <(_hdma_windowbackground_data + 896)
    byte $80 | 112
    word <(_hdma_windowbackground_data + 1344)
    byte 0
    word 0
    byte 0
    word 0

    section "_rodata.far.hdma.scroll"
    global _hdma_scroll_tables
_hdma_scroll_tables:
    ; Table 0
    byte $80 | 32
    word <_hdma_scroll_data
    byte $80 | 32
    word <_hdma_scroll_data
    byte $80 | 32
    word <_hdma_scroll_data
    byte $80 | 32
    word <_hdma_scroll_data
    byte $80 | 32
    word <_hdma_scroll_data
    byte $80 | 32
    word <_hdma_scroll_data
    byte $80 | 32
    word <_hdma_scroll_data
    byte 0
    word 0

    ; Table 1
    byte $80 | 32
    word <(_hdma_scroll_data + 64)
    byte $80 | 32
    word <(_hdma_scroll_data + 64)
    byte $80 | 32
    word <(_hdma_scroll_data + 64)
    byte $80 | 32
    word <(_hdma_scroll_data + 64)
    byte $80 | 32
    word <(_hdma_scroll_data + 64)
    byte $80 | 32
    word <(_hdma_scroll_data + 64)
    byte $80 | 32
    word <(_hdma_scroll_data + 64)
    byte 0
    word 0
