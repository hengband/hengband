#pragma once

#include "util/point-2d.h"
#include <memory>

class ItemEntity;
class PlayerType;
class Chest {
public:
    Chest(PlayerType *player_ptr);
    virtual ~Chest() = default;
    void open(bool scatter, const Pos2D &pos, std::shared_ptr<ItemEntity> chest);
    void fire_trap(const Pos2D &pos, std::shared_ptr<ItemEntity> chest);

private:
    PlayerType *player_ptr;
};
