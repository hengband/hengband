#pragma once

#include "system/angband.h"

struct player_type;
class ObjectZapWandEntity {
public:
    ObjectZapWandEntity(player_type *player_ptr);
    virtual ~ObjectZapWandEntity() = default;

    void execute(INVENTORY_IDX item);

private:
    player_type *player_ptr;

    bool check_can_zap() const;
};
