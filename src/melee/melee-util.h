﻿#pragma once

#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "system/angband.h"
#include "spell/spell-types.h"
#include "system/monster-type-definition.h"

/* monster-attack-monster type*/
typedef struct mam_type {
    int effect_type;
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
    rbm_type method;
    bool explode;
    bool touched;
    concptr act;
    spells_type pt;
    rbe_type effect;
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
} mam_type;

mam_type *initialize_mam_type(player_type *subject_ptr, mam_type *mam_ptr, MONRACE_IDX m_idx, MONRACE_IDX t_idx);
