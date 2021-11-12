#pragma once

#include "system/angband.h"

class PlayerType;
class Chest {
public:
    Chest(PlayerType *player_ptr);
    virtual ~Chest() = default;
    void chest_death(bool scatter, POSITION y, POSITION x, OBJECT_IDX o_idx);
    void chest_trap(POSITION y, POSITION x, OBJECT_IDX o_idx);

private:
    PlayerType *player_ptr;
};
