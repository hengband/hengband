#pragma once

#include "system/angband.h"

class PlayerType;
class ObjectZapWandEntity {
public:
    ObjectZapWandEntity(PlayerType *player_ptr);
    virtual ~ObjectZapWandEntity() = default;

    void execute(INVENTORY_IDX item);

private:
    PlayerType *player_ptr;

    bool check_can_zap() const;
};
