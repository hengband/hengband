#pragma once

#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "system/angband.h"
#include "effect/attribute-types.h"

/* monster-attack-monster type*/
struct monster_type;
struct mam_type {
    int attribute;
    MONRACE_IDX m_idx;
    MONRACE_IDX t_idx;
    monster_type *m_ptr;
    monster_type *t_ptr;
    GAME_TEXT m_name[MAX_NLEN];
    GAME_TEXT t_name[MAX_NLEN];
    HIT_POINT damage;
    bool see_m;
    bool see_t;
    bool see_either;
    POSITION y_saver;
    POSITION x_saver;
    RaceBlowMethodType method;
    bool explode;
    bool touched;
    concptr act;
    AttributeType pt;
    RaceBlowEffectType effect;
    ARMOUR_CLASS ac;
    DEPTH rlev;
    bool blinked;
    bool do_silly_attack;
    HIT_POINT power;
    bool obvious;
    int d_dice;
    int d_side;
    bool known;
    bool fear;
    bool dead;
};

class PlayerType;
mam_type *initialize_mam_type(PlayerType *player_ptr, mam_type *mam_ptr, MONRACE_IDX m_idx, MONRACE_IDX t_idx);
