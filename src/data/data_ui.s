; For Calypsi

    .section cfar,rodata
    .global data_ui_fixed_2bpp_lz4
data_ui_fixed_2bpp_lz4:
    .incbin "ui/ui_fixed_2bpp.bin.lz4"
    .global data_ui_fixed_4bpp_lz4
data_ui_fixed_4bpp_lz4:
    .incbin "ui/ui_fixed_4bpp.bin.lz4"
    .global data_ui_dynamic_hp
data_ui_dynamic_hp:
    .incbin "ui/ui_dynamic_hp.bin"
    .global data_ui_dynamic_textadvance
data_ui_dynamic_textadvance:
    .incbin "ui/ui_dynamic_textadvance.bin"
    .global data_ui_dynamic_selectcursor
data_ui_dynamic_selectcursor:
    .incbin "ui/ui_dynamic_selectcursor.bin"