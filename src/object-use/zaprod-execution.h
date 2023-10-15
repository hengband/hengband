#pragma once

#include "system/angband.h"

class PlayerType;
class ObjectZapRodEntity {
public:
    ObjectZapRodEntity(PlayerType *player_ptr);
    virtual ~ObjectZapRodEntity() = default;

    void execute(INVENTORY_IDX i_idx);

private:
    PlayerType *player_ptr;

    bool check_can_zap();
};
