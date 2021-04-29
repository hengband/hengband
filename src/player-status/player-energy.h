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
class PlayerEnergy {
public:
    PlayerEnergy(player_type *creature_ptr);
    PlayerEnergy() = delete;
    virtual ~PlayerEnergy() = default;
    void update_player_turn_energy(ENERGY need_cost, update_turn_type ut_type);
    void reset_player_turn();
private:
    player_type *creature_ptr;
};
