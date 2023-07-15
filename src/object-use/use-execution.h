#pragma once

#include "system/angband.h"

class PlayerType;
class ObjectUseEntity {
public:
    ObjectUseEntity(PlayerType *player_ptr, INVENTORY_IDX item);
    virtual ~ObjectUseEntity() = default;

    void execute();

private:
    PlayerType *player_ptr;
    INVENTORY_IDX item;

    bool check_can_use();
};
