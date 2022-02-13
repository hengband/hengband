#pragma once

#include <vector>

#include "system/angband.h"
#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

struct monster_race;
struct monster_type;
struct melee_spell_type {
    MONSTER_IDX m_idx;
    POSITION y;
    POSITION x;
    MONSTER_IDX target_idx;
    MonsterAbilityType thrown_spell;
    HIT_POINT dam;
    std::vector<MonsterAbilityType> spells;
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
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
};

class PlayerType;
melee_spell_type *initialize_melee_spell_type(PlayerType *player_ptr, melee_spell_type *ms_ptr, MONSTER_IDX m_idx);
