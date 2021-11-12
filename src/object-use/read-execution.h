#pragma once

#include "system/angband.h"

class PlayerType;
class ObjectReadEntity {
public:
    ObjectReadEntity(PlayerType *player_ptr, INVENTORY_IDX item);
    virtual ~ObjectReadEntity() = default;

    void execute(bool known);

private:
    PlayerType *player_ptr;
    INVENTORY_IDX item;

    bool check_can_read();
};
