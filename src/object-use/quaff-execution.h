#pragma once

#include "system/angband.h"

struct player_type;
class ObjectQuaffEntity {
public:
    ObjectQuaffEntity(player_type *player_ptr);
    virtual ~ObjectQuaffEntity() = default;

    void execute(INVENTORY_IDX item);

private:
    player_type *player_ptr;

    bool check_can_quaff();
    bool booze();
    bool detonation();
};
