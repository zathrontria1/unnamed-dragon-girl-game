; For VBCC

    section "_rodata.far.bindata.0"
    global _data_palette
_data_palette:
    incbin "palette/palette_ui_fixed_4bpp.bin"
    incbin "palette/palette_bg_dungeon_0.bin"
    incbin "palette/palette_bg_dungeon_1.bin"
    incbin "palette/palette_bg_dungeon_2.bin"
    incbin "palette/palette_bg_dungeon_3.bin"
    incbin "palette/palette_bg_dungeon_4.bin"
    incbin "palette/palette_bg_dungeon_5.bin"
    incbin "palette/palette_bg_dungeon_4.bin"
    incbin "palette/palette_spr_player.bin"
    incbin "palette/palette_spr_common0.bin"
    incbin "palette/palette_spr_common1.bin"
    incbin "palette/palette_spr_common1.bin"
    incbin "palette/palette_spr_common1.bin"
    incbin "palette/palette_spr_common1.bin"
    incbin "palette/palette_spr_common1.bin"
    incbin "palette/palette_bg_dungeon_2.bin"
    global _data_palette_map_8bpp
_data_palette_map_8bpp:
    incbin "palette/palette_bg_map_dungeon_8bpp.bin"
    global _data_palette_splash
_data_palette_splash:
    incbin "splash/palette_splash.bin"