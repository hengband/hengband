#pragma once

#include "system/angband.h"

/* Racial Power Info / レイシャル・パワー情報の構造体定義 */
typedef struct rpi_type {
    GAME_TEXT racial_name[MAX_NLEN];
    PLAYER_LEVEL min_level; //!<体得レベル
    int cost;
    int stat;
    PERCENTAGE fail;
    int number;
    int racial_cost;
} rpi_type;

PERCENTAGE racial_chance(player_type *creature_ptr, rpi_type *rpi_ptr);
int racial_aux(player_type *creature_ptr, rpi_type *rpi_ptr);
bool exe_racial_power(player_type *creature_ptr, s32b command);
