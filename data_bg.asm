; For VBCC

    section "_rodata.far.bindata.0"
    global _data_bg_dungeon_lz4
_data_bg_dungeon_lz4:
    incbin "bg/bg_dungeon.bin.lz4"
    global _data_bg_dungeon_anim_water
_data_bg_dungeon_anim_water:
    incbin "bg/bg_dungeon_anim_water.bin"
    global _data_bg_dungeon_anim_torch
_data_bg_dungeon_anim_torch:
    incbin "bg/bg_dungeon_anim_torch.bin"
    global _data_bg_map_dungeon_8bpp_lz4
_data_bg_map_dungeon_8bpp_lz4:
    incbin "bg/bg_map_dungeon_8bpp.bin.lz4"

    section "_rodata.far.bindata.1"
    global  _data_bg_splash
_data_bg_splash:
    incbin "splash/loading_splash_mini_quantized.bin"
    global  _data_tilemap_splash
_data_tilemap_splash:
    incbin "splash/loading_splash_mini_quantized_tilemap.bin"
