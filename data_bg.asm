; For VBCC

    section "_rodata.far.bindata.bgtiles.0"
    global _data_bg_dungeon_lz4
_data_bg_dungeon_lz4:
    incbin "bg/bg_dungeon.bin.lz4"

    section "_rodata.far.bindata.bgtiles.1"
    global _data_bg_dungeon_anim_water_lz4
_data_bg_dungeon_anim_water_lz4:
    incbin "bg/bg_dungeon_anim_water.bin.lz4"

    section "_rodata.far.bindata.bgtiles.2"
    global _data_bg_dungeon_anim_torch_lz4
_data_bg_dungeon_anim_torch_lz4:
    incbin "bg/bg_dungeon_anim_torch.bin.lz4"

    section "_rodata.far.bindata.overviewmaps.0"
    global _data_bg_map_dungeon_0_8bpp_lz4
_data_bg_map_dungeon_0_8bpp_lz4:
    incbin "bg/bg_map_dungeon_0_8bpp.bin.lz4"

    section "_rodata.far.bindata.overviewmaps.1"
    global _data_bg_map_dungeon_1_8bpp_lz4
_data_bg_map_dungeon_1_8bpp_lz4:
    incbin "bg/bg_map_dungeon_1_8bpp.bin.lz4"

    section "_rodata.far.bindata.splash.0"
    global  _data_bg_splash_lz4
_data_bg_splash_lz4:
    incbin "splash/loading_splash_new_quantized.bin.lz4"

    section "_rodata.far.bindata.splash.1"
    global  _data_tilemap_splash_lz4
_data_tilemap_splash_lz4:
    incbin "splash/loading_splash_new_quantized_tilemap.bin.lz4"
