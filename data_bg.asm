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

    section "_rodata.far.bindata.error.0"
    global  _data_bg_error_back_lz4
_data_bg_error_back_lz4:
    incbin "error/error_background.bin.lz4"

    section "_rodata.far.bindata.error.1"
    global  _data_tilemap_error_back_lz4
_data_tilemap_error_back_lz4:
    incbin "error/error_background_tilemap.bin.lz4"
