; For Calypsi

    .section cfar,rodata
    .global data_sprite_player
data_sprite_player:
    .incbin "sprites/spr_player.bin"
    .global data_sprite_fixed_lz4
data_sprite_fixed_lz4:
    .incbin "sprites/spr_fixed.bin.lz4"
    .global data_sprite_slime
data_sprite_slime:
    .incbin "sprites/spr_slime.bin"
    .global data_sprite_spawn_placeholder
data_sprite_spawn_placeholder:
    .incbin "sprites/spr_spawn_placeholder.bin"
    .global data_sprite_drop_coin
data_sprite_drop_coin:
    .incbin "sprites/spr_drop_coin.bin"
