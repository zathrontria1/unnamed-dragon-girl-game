// Terminate metasprites with a 0xffff for size

const struct spr_metaspr_definition data_metaspr_door_ns[] = {
    //{0x6c | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, 0, 0},
    //{0x6e | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 16, 0, 0},
    {0x8c | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -16, 1},
    //{0x4c | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -16, 0},
    //{0x4e | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 16, -16, 0},
    {0x6c | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -32, 0},
    {0x6e | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 16, -32, 0},
    {0, 0, 0, 0xffff},
};

const struct spr_metaspr_definition data_metaspr_door_ew[] = {
    {0xaa | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, 0, 0},
    {0x8a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -16, 0},
    {0x6a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -32, 0},
    {0x4a | PAL_INTERACTABLE_BLOCKER_DOOR << 9 | 2 << 12, 0, -48, 0},
    {0, 0, 0, 0xffff},
};
