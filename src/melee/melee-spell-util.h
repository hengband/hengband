#pragma once

#include "monster-race/race-ability-flags.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <string>
#include <vector>

class MonsterRaceInfo;
class MonsterEntity;
class PlayerType;
struct melee_spell_type {
    melee_spell_type(PlayerType *player_ptr, MONSTER_IDX m_idx);

    POSITION y = 0;
    POSITION x = 0;
    MONSTER_IDX target_idx = 0;
    int dam = 0;
    std::vector<MonsterAbilityType> spells{};
    bool can_remember = false;
    EnumClassFlagGroup<MonsterAbilityType> ability_flags{};
    std::string m_name = "";

    MONSTER_IDX m_idx;
    MonsterAbilityType thrown_spell;

    MonsterEntity *m_ptr;
    MonsterEntity *t_ptr;
    MonsterRaceInfo *r_ptr;
    bool see_m;
    bool maneable;
    bool pet;
    bool in_no_magic_dungeon;
};
