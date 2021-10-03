#pragma once

#include "system/angband.h"

#include <map>

enum class RF_ABILITY;

/* Imitator */
typedef struct monster_power {
    PLAYER_LEVEL level;
    MANA_POINT smana;
    PERCENTAGE fail;
    int manedam;
    int manefail;
    int use_stat;
    concptr name;
} monster_power;

#define MAX_MONSPELLS 96

extern const std::map<RF_ABILITY, const monster_power> monster_powers;
extern const std::map<RF_ABILITY, concptr> monster_powers_short;
