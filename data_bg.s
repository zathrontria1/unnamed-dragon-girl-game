; For Calypsi

    .section cfar,rodata
    .global data_bg_dungeon_lz4
data_bg_dungeon_lz4:
    .incbin "bg/bg_dungeon.bin.lz4"
    .global data_bg_dungeon_anim_water
data_bg_dungeon_anim_water:
    .incbin "bg/bg_dungeon_anim_water.bin"
    .global data_bg_dungeon_anim_torch
data_bg_dungeon_anim_torch:
    .incbin "bg/bg_dungeon_anim_torch.bin"
    .global data_bg_map_dungeon_8bpp_lz4
data_bg_map_dungeon_8bpp_lz4:
    .incbin "bg/bg_map_dungeon_8bpp.bin.lz4"

    .global  data_bg_splash
data_bg_splash:
    .incbin "splash/loading_splash_mini_quantized.bin"
    .global  data_tilemap_splash
data_tilemap_splash:
    .incbin "splash/loading_splash_mini_quantized_tilemap.bin"
