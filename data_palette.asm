; For VBCC

    section "_rodata.far.bindata.palette.maingame.0"
    global _data_palette_blank
_data_palette_blank:
    incbin "palette/palette_bg_blank.bin"

    section "_rodata.far.bindata.palette.maingame.1"
    global _data_palette_ui
_data_palette_ui:
    incbin "palette/palette_ui_fixed_4bpp.bin"

    section "_rodata.far.bindata.palette.maingame.2"
    global _data_palette_dungeon_0
_data_palette_dungeon_0:
    incbin "palette/palette_bg_dungeon_0.bin"

    section "_rodata.far.bindata.palette.maingame.3"
    global _data_palette_dungeon_1
_data_palette_dungeon_1:
    incbin "palette/palette_bg_dungeon_1.bin"

    section "_rodata.far.bindata.palette.maingame.4"
    global _data_palette_dungeon_2
_data_palette_dungeon_2:
    incbin "palette/palette_bg_dungeon_2.bin"

    section "_rodata.far.bindata.palette.maingame.5"
    global _data_palette_player
_data_palette_player:
    incbin "palette/palette_spr_player.bin"

    section "_rodata.far.bindata.palette.maingame.6"
    global _data_palette_common_0
_data_palette_common_0:
    incbin "palette/palette_spr_common0.bin"

    section "_rodata.far.bindata.palette.maingame.7"
    global _data_palette_common_1
_data_palette_common_1:
    incbin "palette/palette_spr_common1.bin"

    section "_rodata.far.bindata.palette.subscreen.0"
    global _data_palette_player_portrait
_data_palette_player_portrait:
    incbin "palette/palette_spr_player_portrait.bin"
    
    section "_rodata.far.bindata.palette.subscreen.1"
    global _data_palette_map_0_8bpp
_data_palette_map_0_8bpp:
    incbin "palette/palette_bg_map_dungeon_0_8bpp.bin"

    section "_rodata.far.bindata.palette.subscreen.2"
    global _data_palette_map_1_8bpp
_data_palette_map_1_8bpp:
    incbin "palette/palette_bg_map_dungeon_1_8bpp.bin"

    section "_rodata.far.bindata.palette.splash.0"
    global _data_palette_splash
_data_palette_splash:
    incbin "splash/palette_splash.bin"