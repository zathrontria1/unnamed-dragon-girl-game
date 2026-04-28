; For VBCC

    section "_rodata.far.bindata.0"
    global _data_ui_fixed_2bpp_lz4
_data_ui_fixed_2bpp_lz4:
    incbin "ui/ui_fixed_2bpp.bin.lz4"
    global _data_ui_fixed_4bpp_lz4
_data_ui_fixed_4bpp_lz4:
    incbin "ui/ui_fixed_4bpp.bin.lz4"
    global _data_ui_dynamic_hp
_data_ui_dynamic_hp:
    incbin "ui/ui_dynamic_hp.bin"
    global _data_ui_dynamic_textadvance
_data_ui_dynamic_textadvance:
    incbin "ui/ui_dynamic_textadvance.bin"
    global _data_ui_dynamic_selectcursor
_data_ui_dynamic_selectcursor:
    incbin "ui/ui_dynamic_selectcursor.bin"