#pragma once

#include "system/angband.h"

#include <map>

enum class MonsterAbilityType;

/* Imitator */
struct monster_power {
    PLAYER_LEVEL level;
    MANA_POINT smana;
    PERCENTAGE fail;
    int manedam;
    int manefail;
    int use_stat;
    concptr name;
};

#define MAX_MONSPELLS 96

extern const std::map<MonsterAbilityType, const monster_power> monster_powers;
extern const std::map<MonsterAbilityType, concptr> monster_powers_short;
