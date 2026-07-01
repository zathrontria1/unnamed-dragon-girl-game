; Place everything that has to be aligned to some boundary here.
; Usually involves data that can be optimized to DP accesses

;NEAR uint8_t spr_depth_count[257]
    global _spr_depth_count
    section "DONTMERGE_bss.near.spr_depth_count.0","aurwz"
    align 5 ; This is in bit count, so be careful
_spr_depth_count:
    reserve 257
    