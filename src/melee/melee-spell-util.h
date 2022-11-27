#pragma once

#include <vector>

#include "monster-race/race-ability-flags.h"
#include "system/angband.h"
#include "util/flag-group.h"

class MonsterRaceInfo;
class MonsterEntity;
struct melee_spell_type {
    MONSTER_IDX m_idx;
    POSITION y;
    POSITION x;
    MONSTER_IDX target_idx;
    MonsterAbilityType thrown_spell;
    int dam;
    std::vector<MonsterAbilityType> spells;
    GAME_TEXT m_name[160];
#ifdef JP
#else
    char m_poss[160];
#endif

    MonsterEntity *m_ptr;
    MonsterEntity *t_ptr;
    MonsterRaceInfo *r_ptr;
    bool see_m;
    bool maneable;
    bool pet;
    bool in_no_magic_dungeon;
    bool can_remember;
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
};

class PlayerType;
melee_spell_type *initialize_melee_spell_type(PlayerType *player_ptr, melee_spell_type *ms_ptr, MONSTER_IDX m_idx);
