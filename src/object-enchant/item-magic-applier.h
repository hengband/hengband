#pragma once

#include "system/angband.h"
#include <tuple>

class ItemEntity;
class PlayerType;
class ItemMagicApplier {
public:
    ItemMagicApplier(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH lev, BIT_FLAGS mode);
    void execute();

private:
    PlayerType *player_ptr;
    ItemEntity *o_ptr;
    DEPTH lev;
    BIT_FLAGS mode;

    std::tuple<int, int> calculate_chances();
    int calculate_power(const int chance_good, const int chance_great);
    int calculate_rolls(const int power);
    void try_make_artifact(const int rolls);
    bool set_fixed_artifact_generation_info();
    void apply_cursed();
};
