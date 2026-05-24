; For Calypsi

    .section cfar,rodata
    .global data_palette
data_palette:
    .incbin "palette/palette_ui_fixed_4bpp.bin"
    .incbin "palette/palette_bg_dungeon_0.bin"
    .incbin "palette/palette_bg_dungeon_1.bin"
    .incbin "palette/palette_bg_dungeon_2.bin"
    .incbin "palette/palette_bg_dungeon_3.bin"
    .incbin "palette/palette_bg_dungeon_4.bin"
    .incbin "palette/palette_bg_dungeon_5.bin"
    .incbin "palette/palette_bg_dungeon_4.bin"
    .incbin "palette/palette_spr_player.bin"
    .incbin "palette/palette_spr_common0.bin"
    .incbin "palette/palette_spr_common1.bin"
    .incbin "palette/palette_spr_common2.bin"
    .incbin "palette/palette_spr_common2.bin"
    .incbin "palette/palette_spr_common2.bin"
    .incbin "palette/palette_spr_player_portrait.bin"
    .incbin "palette/palette_bg_dungeon_2.bin"

    .global data_palette_map_0_8bpp
data_palette_map_0_8bpp:
    .incbin "palette/palette_bg_map_dungeon_0_8bpp.bin"
    .global data_palette_map_1_8bpp
data_palette_map_1_8bpp:
    .incbin "palette/palette_bg_map_dungeon_1_8bpp.bin"

    .global data_palette_splash
data_palette_splash:
    .incbin "splash/palette_splash.bin"