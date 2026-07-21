typedef struct enemy_standard_cfg
{
    uint32_t dist_min;           /* squared distance threshold for AI */
    bool     allow_flipflop;     /* passed to AI process */
    uint16_t attack_delay;       /* passed to AI process */
    uint8_t  archetype;          /* AI archetype */
    uint16_t attack_hitbox_id;   /* OBJID_BUBBLE_E / OBJID_ARROW_E / ... */
    uint16_t attack_sfx;         /* SFX_ATK_SPLASH / SFX_ATK_SWING / ... */
    uint16_t clip_pre_attack;    /* stream clip before spawn, 0 = none */
    uint8_t  vel_multiplier;     /* projectile speed: 1 or 2 * V_MUL */
    bool     set_hitbox_angle;   /* copy temp_angle into hitbox->angle */
    bool     extra_idle_on_attack_special; /* freeze anim for ATTACK_SPECIAL states */
} enemy_standard_cfg_t;

void Routines_Enemy_CommonUpdate(struct game_object * o, const enemy_standard_cfg_t * cfg);

void Routines_Enemy_Slime(struct game_object * o);
void Routines_Enemy_Lizardman(struct game_object * o);
void Routines_Enemy_Slime_Bubble(struct game_object * o);
void Routines_Enemy_Lizardman_Arrow(struct game_object * o);
void Routines_Enemy_InvisibleHit(struct game_object * o);

uint16_t Routines_Enemy_GetFacing(struct game_object * o);

void Routines_Enemy_HandleFailedSpawn(struct game_object * o);
