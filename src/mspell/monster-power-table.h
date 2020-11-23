#pragma once

#include "system/angband.h"

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

extern const monster_power monster_powers[MAX_MONSPELLS];
extern const concptr monster_powers_short[MAX_MONSPELLS];
