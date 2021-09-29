#pragma once

#include "system/angband.h"

struct player_type;
class ObjectZapRodEntity {
public:
    ObjectZapRodEntity(player_type *player_ptr);
    virtual ~ObjectZapRodEntity() = default;

    void execute(INVENTORY_IDX item);

private:
    player_type *player_ptr;

    bool check_can_zap();
};
