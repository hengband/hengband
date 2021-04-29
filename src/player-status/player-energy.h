#pragma once

#include "system/angband.h"

enum class update_turn_type {
    ENERGY_SUBSTITUTION,
    ENERGY_ADDITION,
    ENERGY_SUBTRACTION,
    ENERGY_MULTIPLICATION,
    ENERGY_DIVISION,
};

typedef struct player_type player_type;
void update_player_turn_energy(player_type *creature_ptr, ENERGY need_cost, update_turn_type ut_type);
void reset_player_turn(player_type *creature_ptr);
