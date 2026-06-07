; For Calypsi

    .section cfar,rodata
    .global data_bg_dungeon_lz4
data_bg_dungeon_lz4:
    .incbin "bg/bg_dungeon.bin.lz4"
    .global data_bg_dungeon_anim_water_lz4
data_bg_dungeon_anim_water_lz4:
    .incbin "bg/bg_dungeon_anim_water.bin.lz4"
    .global data_bg_dungeon_anim_torch_lz4
data_bg_dungeon_anim_torch_lz4:
    .incbin "bg/bg_dungeon_anim_torch.bin.lz4"

    .global data_bg_map_dungeon_0_8bpp_lz4
data_bg_map_dungeon_0_8bpp_lz4:
    .incbin "bg/bg_map_dungeon_0_8bpp.bin.lz4"
    .global data_bg_map_dungeon_1_8bpp_lz4
data_bg_map_dungeon_1_8bpp_lz4:
    .incbin "bg/bg_map_dungeon_1_8bpp.bin.lz4"

    .global  data_bg_splash_lz4
data_bg_splash_lz4:
    .incbin "splash/loading_splash_mini_quantized.bin.lz4"
    .global  data_tilemap_splash_lz4
data_tilemap_splash_lz4:
    .incbin "splash/loading_splash_mini_quantized_tilemap.bin.lz4"
