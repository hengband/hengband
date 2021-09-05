#pragma once

#include <vector>

#include "system/angband.h"
#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

struct monster_race;
struct monster_type;
typedef struct melee_spell_type {
    MONSTER_IDX m_idx;
    POSITION y;
    POSITION x;
    MONSTER_IDX target_idx;
    RF_ABILITY thrown_spell;
    HIT_POINT dam;
    std::vector<RF_ABILITY> spells;
    GAME_TEXT m_name[160];
#ifdef JP
#else
    char m_poss[160];
#endif

    monster_type *m_ptr;
    monster_type *t_ptr;
    monster_race *r_ptr;
    bool see_m;
    bool maneable;
    bool pet;
    bool in_no_magic_dungeon;
    bool can_remember;
    EnumClassFlagGroup<RF_ABILITY> ability_flags;
} melee_spell_type;

struct player_type;
melee_spell_type *initialize_melee_spell_type(player_type *target_ptr, melee_spell_type *ms_ptr, MONSTER_IDX m_idx);
