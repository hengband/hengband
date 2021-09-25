#pragma once

#include "system/angband.h"

struct player_type;
class ObjectReadEntity {
public:
    ObjectReadEntity(player_type *player_ptr, INVENTORY_IDX item);
    virtual ~ObjectReadEntity() = default;

    void execute(bool known);

private:
    player_type *player_ptr;
    INVENTORY_IDX item;

    bool check_can_read();
};
