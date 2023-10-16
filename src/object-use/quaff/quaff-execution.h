#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
class ObjectQuaffEntity {
public:
    ObjectQuaffEntity(PlayerType *player_ptr);
    virtual ~ObjectQuaffEntity() = default;

    void execute(INVENTORY_IDX i_idx);

private:
    PlayerType *player_ptr;

    bool can_influence();
    bool can_quaff();
    ItemEntity copy_object(const INVENTORY_IDX i_idx);
    void moisten(const ItemEntity &o_ref);
    void change_virtue_as_quaff(const ItemEntity &o_ref);
};
