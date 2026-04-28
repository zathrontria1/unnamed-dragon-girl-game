; For VBCC

    section "_rodata.far.bindata.0"
    global _data_sprite_player
_data_sprite_player:
    incbin "sprites/spr_player.bin"
    global _data_sprite_fixed_lz4
_data_sprite_fixed_lz4:
    incbin "sprites/spr_fixed.bin.lz4"
    global _data_sprite_slime
_data_sprite_slime:
    incbin "sprites/spr_slime.bin"
    global _data_sprite_spawn_placeholder
_data_sprite_spawn_placeholder:
    incbin "sprites/spr_spawn_placeholder.bin"
    global _data_sprite_drop_coin
_data_sprite_drop_coin:
    incbin "sprites/spr_drop_coin.bin"
