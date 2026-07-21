#ifndef DATA_METASPR_LIST_H
#define DATA_METASPR_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include "defs_objects.h"
#include "defs_structs.h"
#include "spr_metaspr.h"

// Terminate metasprites with a 0xffff for size
const struct spr_metaspr_definition data_metaspr_door_ns_closed[] = {
    {0x8c | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -16, 1},
    {0x8a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -32, 0},
    {0x8a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 16, -32, 0},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_door_ns_open[] = {
    {0x4c | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -16, 1},
    {0x8a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -32, 0},
    {0x8a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 16, -32, 0},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_door_ew_closed[] = {
    {0xaa | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, 0, 0},
    {0xaa | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -16, 0},
    {0xaa | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -32, 0},
    {0xaa | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -48, 0},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_door_ew_open[] = {
    {0x6a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, 0, 0},
    {0x6a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -16, 0},
    {0x6a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -32, 0},
    {0x6a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -48, 0},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_door_ew_open_flip[] = {
    {0x6a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12 | true << 14, 0, 0, 0},
    {0x6a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12 | true << 14, 0, -16, 0},
    {0x6a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12 | true << 14, 0, -32, 0},
    {0x6a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12 | true << 14, 0, -48, 0},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_level_warp_closed[] = {
    {0x8c | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -16, 1},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_shadow_64x16[] = {
    {0xa6 | PAL_FX_SHADOW << 9 | 2 << 12, 0, 0, 0},
    {0xa8 | PAL_FX_SHADOW << 9 | 2 << 12, 16, 0, 0},
    {0xa8 | PAL_FX_SHADOW << 9 | true << 14 | 2 << 12, 32, 0, 0},
    {0xa6 | PAL_FX_SHADOW << 9 | true << 14 | 2 << 12, 48, 0, 0},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_shadow_32x16[] = {
    {0xa6 | PAL_FX_SHADOW << 9 | 2 << 12, 0, 0, 0},
    {0xa6 | PAL_FX_SHADOW << 9 | true << 14 | 2 << 12, 16, 0, 0},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_boss_generic_64x96[] = {
    {0x14e | PAL_BOSS_TEST << 9 | 2 << 12, 48, 0, 0},
    {0x14c | PAL_BOSS_TEST << 9 | 2 << 12, 32, 0, 0},
    {0x14a | PAL_BOSS_TEST << 9 | 2 << 12, 16, 0, 0},
    {0x148 | PAL_BOSS_TEST << 9 | 2 << 12, 0, 0, 0},

    {0x108 | PAL_BOSS_TEST << 9 | 2 << 12, 0, -32, 1},
    {0x10c | PAL_BOSS_TEST << 9 | 2 << 12, 32, -32, 1},

    {0x146 | PAL_BOSS_TEST << 9 | 2 << 12, 48, -48, 0},
    {0x144 | PAL_BOSS_TEST << 9 | 2 << 12, 32, -48, 0},
    {0x142 | PAL_BOSS_TEST << 9 | 2 << 12, 16, -48, 0},
    {0x140 | PAL_BOSS_TEST << 9 | 2 << 12, 0, -48, 0},

    {0x100 | PAL_BOSS_TEST << 9 | 2 << 12, 0, -80, 1},
    {0x104 | PAL_BOSS_TEST << 9 | 2 << 12, 32, -80, 1},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_boss_generic_64x96_hflip[] = {
    {0x14e | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 0, 0, 0},
    {0x14c | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 16, 0, 0},
    {0x14a | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 32, 0, 0},
    {0x148 | PAL_BOSS_TEST << 9 | 2 << 12, 48, 0, 0},

    {0x108 | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 32, -32, 1},
    {0x10c | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 0, -32, 1},

    {0x146 | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 0, -48, 0},
    {0x144 | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 16, -48, 0},
    {0x142 | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 32, -48, 0},
    {0x140 | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 48, -48, 0},

    {0x100 | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 32, -80, 1},
    {0x104 | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 0, -80, 1},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_boss_generic_hands_l[] = {
    {0x184 | PAL_BOSS_TEST << 9 | 2 << 12, 40, -36, 1}, // Inner

    {0, 0, 0, 0xffff},
};


const struct spr_metaspr_definition data_metaspr_boss_generic_hands_r[] = {
    {0x180 | PAL_BOSS_TEST << 9 | 2 << 12, -8, -28, 1}, // Outer

    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_boss_generic_hands_hflip_l[] = {
    {0x180 | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, 40, -28, 1}, // Outer

    {0, 0, 0, 0xffff},
};


const struct spr_metaspr_definition data_metaspr_boss_generic_hands_hflip_r[] = {
    {0x184 | PAL_BOSS_TEST << 9 | true << 14 | 2 << 12, -8, -36, 1}, // Inner

    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_shadow_hands[] = {
    {0xa6 | PAL_FX_SHADOW << 9 | 2 << 12, -8, 4, 0},
    {0xa6 | PAL_FX_SHADOW << 9 | true << 14 | 2 << 12, 8, 4, 0},

    {0xa6 | PAL_FX_SHADOW << 9 | 2 << 12, 40, -4, 0},
    {0xa6 | PAL_FX_SHADOW << 9 | true << 14 | 2 << 12, 56, -4, 0},

    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_shadow_hands_hflip[] = {
    {0xa6 | PAL_FX_SHADOW << 9 | 2 << 12, -8, -4, 0},
    {0xa6 | PAL_FX_SHADOW << 9 | true << 14 | 2 << 12, 8, -4, 0},

    {0xa6 | PAL_FX_SHADOW << 9 | 2 << 12, 40, 4, 0},
    {0xa6 | PAL_FX_SHADOW << 9 | true << 14 | 2 << 12, 56, 4, 0},

    {0, 0, 0, 0xffff},
};

#endif // DATA_METASPR_LIST_H

