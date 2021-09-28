#pragma once

#include "system/angband.h"

struct player_type;
class Chest {
public:
    Chest(player_type *player_ptr);
    virtual ~Chest() = default;
    void chest_death(bool scatter, POSITION y, POSITION x, OBJECT_IDX o_idx);
    void chest_trap(POSITION y, POSITION x, OBJECT_IDX o_idx);

private:
    player_type *player_ptr;
};
