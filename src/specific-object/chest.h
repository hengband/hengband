#pragma once

#include "util/point-2d.h"

class PlayerType;
class Chest {
public:
    Chest(PlayerType *player_ptr);
    virtual ~Chest() = default;
    void open(bool scatter, const Pos2D &pos, short item_idx);
    void fire_trap(const Pos2D &pos, short item_idx);

private:
    PlayerType *player_ptr;
};
