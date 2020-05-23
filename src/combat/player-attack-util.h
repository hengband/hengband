#pragma once

#include "system/angband.h"
#include "combat/combat-options-type.h"

typedef struct player_attack_type {
    s16b hand;
    combat_options mode;
    monster_type *m_ptr;
    bool backstab;
    bool suprise_attack;
    bool stab_fleeing;
    bool monk_attack;
    int num_blow;
    HIT_POINT attack_damage;
    GAME_TEXT m_name[MAX_NLEN];
    BIT_FLAGS flags[TR_FLAG_SIZE];
} player_attack_type;
