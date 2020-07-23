#pragma once

#include "system/angband.h"

// Monster Spell Remover.
typedef struct monster_race monster_race;
typedef struct msr_type {
    monster_race *r_ptr;
    u32b f4;
    u32b f5;
    u32b f6;
    u32b smart;
} msr_type;

msr_type *initialize_msr_type(player_type *target_ptr, msr_type *msr_ptr, MONSTER_IDX m_idx, const u32b f4p, const u32b f5p, const u32b f6p);
bool int_outof(monster_race *r_ptr, PERCENTAGE prob);
