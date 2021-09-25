#pragma once

#include "system/angband.h"

struct player_type;
class ObjectUseEntity {
public:
    ObjectUseEntity(player_type *player_ptr, INVENTORY_IDX item);
    virtual ~ObjectUseEntity() = default;

    void execute();

private:
    player_type *player_ptr;
    INVENTORY_IDX item;

    bool check_can_use();
};
