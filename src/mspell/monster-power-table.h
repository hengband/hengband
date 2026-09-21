#pragma once

#include <map>
#include <string_view>

enum class MonsterAbilityType;

/* Imitator */
struct monster_power {
    short level = 0;
    int smana = 0;
    int fail = 0;
    int manedam = 0;
    int manefail = 0;
    int use_stat = 0;
    std::string_view name;
};

#define MAX_MONSPELLS 96

extern const std::map<MonsterAbilityType, const monster_power> monster_powers;
