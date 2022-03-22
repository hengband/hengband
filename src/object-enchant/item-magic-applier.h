#pragma once

#include "system/angband.h"
#include <tuple>

class ObjectType;
class PlayerType;
class ItemMagicApplier {
public:
    ItemMagicApplier(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH lev, BIT_FLAGS mode);
    void execute();

private:
    PlayerType *player_ptr;
    ObjectType *o_ptr;
    DEPTH lev;
    BIT_FLAGS mode;

    std::tuple<int, int> calculate_chances();
};
