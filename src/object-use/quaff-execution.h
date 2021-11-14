#pragma once

#include "system/angband.h"

class PlayerType;
class ObjectQuaffEntity {
public:
    ObjectQuaffEntity(PlayerType *player_ptr);
    virtual ~ObjectQuaffEntity() = default;

    void execute(INVENTORY_IDX item);

private:
    PlayerType *player_ptr;

    bool check_can_quaff();
    bool booze();
    bool detonation();
};
