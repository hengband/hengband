#pragma once

#include "system/angband.h"

class PlayerType;
class PlayerEnergy {
public:
    PlayerEnergy(PlayerType *player_ptr);
    virtual ~PlayerEnergy() = default;
    void set_player_turn_energy(ENERGY need_cost); // 代入.
    void add_player_turn_energy(ENERGY need_cost); // 加算.
    void sub_player_turn_energy(ENERGY need_cost); // 減算.
    void mul_player_turn_energy(ENERGY need_cost); // 乗算.
    void div_player_turn_energy(ENERGY need_cost); // 除算.
    void reset_player_turn();

private:
    PlayerType *player_ptr;
};
